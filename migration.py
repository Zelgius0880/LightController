import os
import argparse
import requests
import json
from dotenv import load_dotenv

# Load environment variables from .env file
load_dotenv()

HUE_LIGHTS_CACHE = {}
MISSING_HUE_IDS = {}  # {v1_id: name}
MIGRATED_IDS_MAP = {} # {v1_id: v2_id}

def fetch_hue_lights_from_bridge():
    """
    Fetches Hue lights from the Bridge API v2 directly.
    """
    global HUE_LIGHTS_CACHE
    bridge_ip = os.getenv("HUE_BRIDGE_IP")
    username = os.getenv("HUE_BRIDGE_USERNAME")
    if not bridge_ip or not username:
        print("Warning: Hue Bridge configuration missing in .env.")
        return

    url = f"https://{bridge_ip}/clip/v2/resource/light"
    headers = {
        "hue-application-key": username
    }
    try:
        # Note: verify=False because Hue Bridge uses self-signed certs
        response = requests.get(url, headers=headers, verify=False)
        if response.status_code == 200:
            data = response.json()
            for light in data.get("data", []):
                id_v1 = light.get("id_v1")
                if id_v1:
                    HUE_LIGHTS_CACHE[id_v1] = light
            print(f"Fetched {len(HUE_LIGHTS_CACHE)} lights from Hue Bridge API.")
        else:
            print(f"Warning: Failed to fetch Hue lights: {response.status_code} {response.text}")
    except Exception as e:
        print(f"Warning: Error fetching Hue lights from bridge: {e}")

def update_hue_light_name(light_id, name):
    """
    Updates the name of a Hue light via the Bridge API v2.
    """
    bridge_ip = os.getenv("HUE_BRIDGE_IP")
    username = os.getenv("HUE_BRIDGE_USERNAME")
    if not bridge_ip or not username:
        return

    url = f"https://{bridge_ip}/clip/v2/resource/light/{light_id}"
    headers = {
        "hue-application-key": username
    }
    payload = {
        "metadata": {
            "name": name
        }
    }
    try:
        # Note: verify=False because Hue Bridge uses self-signed certs
        response = requests.put(url, headers=headers, json=payload, verify=False)
        if response.status_code == 200:
            print(f"  - Successfully updated Hue light {light_id} name to '{name}'")
        else:
            print(f"  - Failed to update Hue light {light_id} name: {response.status_code} {response.text}")
    except Exception as e:
        print(f"  - Error updating Hue light {light_id} name: {e}")

def get_id_token(api_key, email, password):
    """
    Authenticates with Firebase Auth and returns an ID token.
    """
    url = f"https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key={api_key}"
    payload = {
        "email": email,
        "password": password,
        "returnSecureToken": True
    }
    response = requests.post(url, json=payload)
    response.raise_for_status()
    return response.json()["idToken"]

def hsb_to_xy(h, s, b):
    """
    Converts HSB (Hue, Saturation, Brightness) to CIE XY coordinates.
    h: 0-360, s: 0-100, b: 0-100
    Returns (x, y)
    """
    h = float(h) / 360.0
    s = float(s) / 100.0
    v = float(b) / 100.0

    if s == 0:
        r = g = b_val = v
    else:
        i = int(h * 6)
        f = (h * 6) - i
        p = v * (1 - s)
        q = v * (1 - s * f)
        t = v * (1 - s * (1 - f))
        i %= 6
        if i == 0: r, g, b_val = v, t, p
        elif i == 1: r, g, b_val = q, v, p
        elif i == 2: r, g, b_val = p, v, t
        elif i == 3: r, g, b_val = p, q, v
        elif i == 4: r, g, b_val = t, p, v
        elif i == 5: r, g, b_val = v, p, q

    # Gamma correction
    r = ((r + 0.055) / (1.0 + 0.055))**2.4 if r > 0.04045 else r / 12.92
    g = ((g + 0.055) / (1.0 + 0.055))**2.4 if g > 0.04045 else g / 12.92
    b_val = ((b_val + 0.055) / (1.0 + 0.055))**2.4 if b_val > 0.04045 else b_val / 12.92

    # Wide Gamut D65 conversion
    X = r * 0.664511 + g * 0.154324 + b_val * 0.162028
    Y = r * 0.283881 + g * 0.668433 + b_val * 0.047685
    Z = r * 0.000088 + g * 0.072310 + b_val * 0.986039

    if (X + Y + Z) == 0:
        return 0.0, 0.0

    x = X / (X + Y + Z)
    y = Y / (X + Y + Z)

    return round(x, 4), round(y, 4)

def transform_data(fields):
    """
    Apply data manipulation between source and destination.
    """
    # fields is a dict where keys are field names and values are {'stringValue': '...', 'integerValue': '...', etc.}
    item_type = fields.get('itemType', {}).get('stringValue')
    type_field = fields.get('type', {}).get('stringValue')

    if item_type == 'LIGHT' and type_field == 'HUE':
        # Retrieve v1 ID
        v1_id = fields.get('id', {}).get('stringValue')
        v1_name = fields.get('name', {}).get('stringValue')
        if v1_id:
            hue_id_v1 = f"/lights/{v1_id}"
            if hue_id_v1 in HUE_LIGHTS_CACHE:
                hue_data = HUE_LIGHTS_CACHE[hue_id_v1]
                v2_id = hue_data.get('id')
                v2_name = hue_data.get('metadata', {}).get('name')

                # Update uid to v2_id
                fields['uid'] = {'stringValue': v2_id}
                print(f"    * Matched HUE light {v1_id} -> {v2_id}")
                MIGRATED_IDS_MAP[v1_id] = v2_id

                # Check and sync name if necessary
                if v1_name and v2_name and v1_name != v2_name:
                    print(f"    * Name mismatch: v1='{v1_name}', v2='{v2_name}'. Updating v2...")
                    update_hue_light_name(v2_id, v1_name)
            else:
                print(f"    ! Warning: No matching Hue light found for v1 id {v1_id}. Skipping.")
                MISSING_HUE_IDS[v1_id] = v1_name or "Unknown"
                return None

        # Convert HSB to XY if present
        if 'hue' in fields or 'saturation' in fields:
            h = int(fields.get('hue', {}).get('integerValue', 0))
            s = int(fields.get('saturation', {}).get('integerValue', 0))
            b = int(fields.get('brightness', {}).get('integerValue', 100))
            
            x, y = hsb_to_xy(h, s, b)
            fields['x'] = {'doubleValue': x}
            fields['y'] = {'doubleValue': y}
            print(f"    * Converted HSB({h}, {s}, {b}) to XY({x}, {y})")

            # Remove hue and saturation, keep brightness
            if 'hue' in fields: del fields['hue']
            if 'saturation' in fields: del fields['saturation']

        # Scale brightness from 1-254 range to 0-100 range
        if 'brightness' in fields:
            old_b_val = fields.get('brightness')
            if 'integerValue' in old_b_val:
                old_b = int(old_b_val['integerValue'])
                # New range 0-100, assuming 254 is max
                new_b = int(round((old_b / 254.0) * 100))
                fields['brightness'] = {'integerValue': str(new_b)}
                print(f"    * Scaled brightness: {old_b} -> {new_b}")

    # Remove the 'id' field as requested for all documents
    if 'id' in fields:
        del fields['id']

    return fields

def write_document(project_id, id_token, doc_path, fields):
    """
    Writes a document to the destination Firestore using the REST API.
    """
    # doc_path is like 'collection/docId' or 'coll/doc/subcoll/subdoc'
    url = f"https://firestore.googleapis.com/v1/projects/{project_id}/databases/(default)/documents/{doc_path}"
    headers = {
        "Authorization": f"Bearer {id_token}"
    }
    
    # We use PATCH with ?currentDocument.exists=false to create OR just PATCH to overwrite
    # Using PATCH allows us to specify the document ID at the end of the URL.
    payload = {
        "fields": fields
    }
    
    print(f"  - Writing document to destination: {doc_path}")
    response = requests.patch(url, headers=headers, json=payload)
    response.raise_for_status()

def migrate_collection(src_config, dest_config, collection_path, indent=0):
    """
    Recursively migrates all documents and subcollections from source to destination.
    """
    prefix = "  " * indent
    src_project_id = src_config['project_id']
    src_token = src_config['token']
    
    dest_project_id = dest_config['project_id']
    dest_token = dest_config['token']

    # Use pageToken for pagination support
    next_page_token = None
    
    print(f"{prefix}Migrating Collection: {collection_path}")

    while True:
        url = f"https://firestore.googleapis.com/v1/projects/{src_project_id}/databases/(default)/documents/{collection_path}"
        params = {}
        if next_page_token:
            params['pageToken'] = next_page_token
        
        headers = {
            "Authorization": f"Bearer {src_token}"
        }

        try:
            response = requests.get(url, headers=headers, params=params)
            
            if response.status_code == 404:
                print(f"{prefix}  (Collection not found in source: {collection_path})")
                return
            
            response.raise_for_status()
            data = response.json()
        except Exception as e:
            print(f"{prefix}  Error fetching collection {collection_path}: {e}")
            return
        
        documents = data.get("documents", [])
        
        if not documents and not next_page_token:
            print(f"{prefix}  (No documents found in source)")
            return

        for doc in documents:
            doc_name = doc["name"] # projects/{project_id}/databases/(default)/documents/{path}
            relative_path = doc_name.split("/documents/")[-1]
            doc_id = relative_path.split("/")[-1]
            
            print(f"{prefix}  - Processing Document: {doc_id}")
            
            # 1. Transform data
            src_fields = doc.get('fields', {})
            transformed_fields = transform_data(src_fields)
            
            # Skip writing if transform_data returns None (e.g. missing light)
            if transformed_fields is None:
                print(f"{prefix}  ! Skipping Document: {doc_id} (Missing required mapping)")
                continue

            # 2. Write to destination
            write_document(dest_project_id, dest_token, relative_path, transformed_fields)
            
            # 3. Recursively migrate subcollections
            # According to the user, 'items' documents don't have subcollections.
            # We only migrate subcollections for documents in the 'groups' collection.
            if collection_path == 'groups':
                # Explicitly migrate the 'items' subcollection
                sub_coll_path = f"{relative_path}/items"
                migrate_collection(src_config, dest_config, sub_coll_path, indent + 2)
            elif '/items/' in relative_path:
                # Based on the path structure groups/{group_id}/items/{item_id}, 
                # these documents are items and don't have further subcollections.
                pass
            else:
                # Fallback for any other potential subcollections
                migrate_subcollections(src_config, dest_config, relative_path, indent + 2)

        next_page_token = data.get("nextPageToken")
        if not next_page_token:
            break

def migrate_subcollections(src_config, dest_config, doc_relative_path, indent):
    """
    Lists subcollections of a document in source and migrates them.
    """
    src_project_id = src_config['project_id']
    src_token = src_config['token']
    
    url = f"https://firestore.googleapis.com/v1/projects/{src_project_id}/databases/(default)/documents/{doc_relative_path}:listCollectionIds"
    headers = {
        "Authorization": f"Bearer {src_token}"
    }
    
    response = requests.post(url, headers=headers)
    if response.status_code != 200:
        # Ignore subcollection listing errors for documents without subcollections (like items)
        return

    collection_ids = response.json().get("collectionIds", [])
    for coll_id in collection_ids:
        sub_coll_path = f"{doc_relative_path}/{coll_id}"
        migrate_collection(src_config, dest_config, sub_coll_path, indent)

def verify_migration(src_config, dest_config, collection_path, report_f):
    """
    Verifies that all documents from source exist in destination.
    """
    src_project_id = src_config['project_id']
    src_token = src_config['token']
    dest_project_id = dest_config['project_id']
    dest_token = dest_config['token']

    print(f"    - Verifying collection: {collection_path}")
    next_page_token = None
    while True:
        url = f"https://firestore.googleapis.com/v1/projects/{src_project_id}/databases/(default)/documents/{collection_path}"
        params = {}
        if next_page_token:
            params['pageToken'] = next_page_token
        headers = {"Authorization": f"Bearer {src_token}"}
        
        response = requests.get(url, headers=headers, params=params)
        if response.status_code == 404:
            return
        response.raise_for_status()
        data = response.json()
        
        for doc in data.get("documents", []):
            # doc["name"] is 'projects/{project_id}/databases/(default)/documents/{path}'
            full_name = doc["name"]
            relative_path = full_name.split("/documents/")[-1]
            
            # Check if doc exists in destination
            dest_url = f"https://firestore.googleapis.com/v1/projects/{dest_project_id}/databases/(default)/documents/{relative_path}"
            dest_headers = {"Authorization": f"Bearer {dest_token}"}
            dest_resp = requests.get(dest_url, headers=dest_headers)
            
            if dest_resp.status_code == 200:
                report_f.write(f"OK: {relative_path}\n")
            else:
                report_f.write(f"MISSING: {relative_path} (Status: {dest_resp.status_code})\n")
                print(f"    ! Missing in destination: {relative_path}")

            # If it's a group, verify its items subcollection
            # collection_path could be 'groups'
            if collection_path == 'groups':
                sub_coll = f"{relative_path}/items"
                verify_migration(src_config, dest_config, sub_coll, report_f)

        next_page_token = data.get("nextPageToken")
        if not next_page_token:
            break

def main():
    parser = argparse.ArgumentParser(description="Migrate data between two Firebase Firestore databases.")
    parser.add_argument("--collection", required=True, help="The name of the collection to migrate.")
    
    args = parser.parse_args()

    # Get Source config from environment
    src_api_key = os.getenv("SOURCE_FIREBASE_API_KEY")
    src_email = os.getenv("SOURCE_FIREBASE_EMAIL")
    src_password = os.getenv("SOURCE_FIREBASE_PASSWORD")
    src_project_id = os.getenv("SOURCE_FIREBASE_PROJECT_ID")

    # Get Destination config from environment
    dest_api_key = os.getenv("DEST_FIREBASE_API_KEY")
    dest_email = os.getenv("DEST_FIREBASE_EMAIL")
    dest_password = os.getenv("DEST_FIREBASE_PASSWORD")
    dest_project_id = os.getenv("DEST_FIREBASE_PROJECT_ID")

    if not all([src_api_key, src_email, src_password, src_project_id,
                dest_api_key, dest_email, dest_password, dest_project_id]):
        print("Error: Missing environment variables in .env file.")
        print("Required: SOURCE_ and DEST_ variants of FIREBASE_API_KEY, EMAIL, PASSWORD, PROJECT_ID")
        return

    try:
        fetch_hue_lights_from_bridge()
        print("Authenticating with Source...")
        src_token = get_id_token(src_api_key, src_email, src_password)
        
        print("Authenticating with Destination...")
        dest_token = get_id_token(dest_api_key, dest_email, dest_password)

        src_config = {'project_id': src_project_id, 'token': src_token}
        dest_config = {'project_id': dest_project_id, 'token': dest_token}
        
        print(f"Starting migration for collection: {args.collection}")
        migrate_collection(src_config, dest_config, args.collection)
        print("Migration completed.")
        
        print("Starting verification...")
        with open("migration_verification.log", "w", encoding="utf-8") as f:
            f.write(f"Verification Report for collection: {args.collection}\n")
            f.write("-" * 50 + "\n")
            verify_migration(src_config, dest_config, args.collection, f)
        print("Verification completed. Results written to migration_verification.log")

        if MISSING_HUE_IDS:
            with open("missing_ids.log", "w", encoding="utf-8") as f:
                for mid in sorted(MISSING_HUE_IDS.keys()):
                    name = MISSING_HUE_IDS[mid]
                    f.write(f"v1 id: {mid}, Name: {name}\n")
            print(f"Log of {len(MISSING_HUE_IDS)} missing Hue IDs written to missing_ids.log")
        
        if MIGRATED_IDS_MAP:
            with open("id_mapping.log", "w", encoding="utf-8") as f:
                for v1, v2 in sorted(MIGRATED_IDS_MAP.items()):
                    f.write(f"v1: {v1} -> v2: {v2}\n")
            print(f"ID mapping written to id_mapping.log")
        
    except Exception as e:
        print(f"Error during migration: {e}")

if __name__ == "__main__":
    main()
