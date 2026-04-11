#ifndef LIGHTCONTROLLER_HUEMODELS_H
#define LIGHTCONTROLLER_HUEMODELS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

namespace Hue {

struct ResourceIdentifier {
    String rid;
    String rtype;

    static ResourceIdentifier fromJson(const JsonVariantConst json) {
        ResourceIdentifier ri;
        if (json.is<JsonObjectConst>()) {
            ri.rid = json["rid"].as<String>();
            ri.rtype = json["rtype"].as<String>();
        } else if (json.is<String>()) {
            ri.rid = json.as<String>();
        }
        return ri;
    }
};

struct Resource {
    String id;
    String type;
    String id_v1;
    ResourceIdentifier owner;

    static void fillResource(Resource& r, const JsonVariantConst json) {
        r.id = json["id"].as<String>();
        r.type = json["type"].as<String>();
        r.id_v1 = json["id_v1"].as<String>();
        if (json["owner"].is<JsonVariantConst>()) {
            r.owner = ResourceIdentifier::fromJson(json["owner"]);
        }
    }

    static Resource fromJson(JsonVariantConst json) {
        Resource r;
        fillResource(r, json);
        return r;
    }
};

struct Device : Resource {
    struct ProductData {
        String model_id;
        String manufacturer_name;
        String product_name;
        String product_archetype;
        String certified;
        String software_version;
        String hardware_platform_type;
    };

    struct Metadata {
        String name;
        String archetype;
    };

    ProductData product_data;
    Metadata metadata;
    std::vector<ResourceIdentifier> services;

    static Device fromJson(const JsonVariantConst json) {
        Device d;
        fillResource(d, json);
        
        d.product_data.model_id = json["product_data"]["model_id"].as<String>();
        d.product_data.manufacturer_name = json["product_data"]["manufacturer_name"].as<String>();
        d.product_data.product_name = json["product_data"]["product_name"].as<String>();
        d.product_data.product_archetype = json["product_data"]["product_archetype"].as<String>();
        d.product_data.certified = json["product_data"]["certified"].as<String>();
        d.product_data.software_version = json["product_data"]["software_version"].as<String>();
        d.product_data.hardware_platform_type = json["product_data"]["hardware_platform_type"].as<String>();

        d.metadata.name = json["metadata"]["name"].as<String>();
        d.metadata.archetype = json["metadata"]["archetype"].as<String>();

        const auto svcs = json["services"].as<JsonArrayConst>();
        for (const JsonVariantConst s : svcs) {
            d.services.push_back(ResourceIdentifier::fromJson(s));
        }

        return d;
    }
};

struct Light :  Resource {
    struct Metadata {
        String name;
        String archetype;
        String function;
    };

    struct On {
        bool on = false;
    };

    struct Dimming {
        float brightness = 0.0f;
        float min_dim_level = 0.0f;
    };

    struct ColorTemperature {
        int mirek = 0;
        bool mirek_valid = false;
    };

    struct Color {
        float x = 0.0f;
        float y = 0.0f;
    };

    Metadata metadata;
    On on;
    Dimming dimming;
    ColorTemperature color_temperature;
    Color color;

    static Light fromJson(JsonVariantConst json) {
        Light l;
        fillResource(l, json);

        l.metadata.name = json["metadata"]["name"].as<String>();
        l.metadata.archetype = json["metadata"]["archetype"].as<String>();
        l.metadata.function = json["metadata"]["function"].as<String>();

        l.on.on = json["on"]["on"].as<bool>();
        
        if (json["dimming"].is<JsonVariantConst>()) {
            l.dimming.brightness = json["dimming"]["brightness"].as<float>();
            l.dimming.min_dim_level = json["dimming"]["min_dim_level"].as<float>();
        }

        if (json["color_temperature"].is<JsonVariantConst>()) {
            l.color_temperature.mirek = json["color_temperature"]["mirek"].as<int>();
            l.color_temperature.mirek_valid = json["color_temperature"]["mirek_valid"].as<bool>();
        }

        if (json["color"].is<JsonVariantConst>()) {
            l.color.x = json["color"]["xy"]["x"].as<float>();
            l.color.y = json["color"]["xy"]["y"].as<float>();
        }

        return l;
    }
};

struct Error {
    String description;
};

template<typename T>
struct Response {
    int status = 200;
    Error errors;
    std::vector<T> data;

    static Response fromJson(const JsonVariantConst json, int status = 200) {
        Response res;
        res.status = status;
        
        if (json.isNull()) return res;

        const auto errs = json["errors"].as<JsonArrayConst>();
        for (JsonVariantConst e : errs) {
           res.errors.description += "\n" + e["description"].as<String>();
        }

        const auto dataArr = json["data"].as<JsonArrayConst>();
        for (JsonVariantConst d : dataArr) {
            res.data.push_back(T::fromJson(d));
        }

        return res;
    }
};

struct AuthSuccess {
    String username;
    String clientKey;

    static AuthSuccess fromJson(JsonVariantConst json) {
        AuthSuccess as;
        if (json["success"].is<JsonVariantConst>()) {
            as.username = json["success"]["username"].as<String>();
            as.clientKey = json["success"]["clientkey"].as<String>();
        }
        return as;
    }
};

struct AuthError {
    int type = 0;
    String address;
    String description;

    static AuthError fromJson(JsonVariantConst json) {
        AuthError ae;
        if (json["error"].is<JsonVariantConst>()) {
            ae.type = json["error"]["type"].as<int>();
            ae.address = json["error"]["address"].as<String>();
            ae.description = json["error"]["description"].as<String>();
        }
        return ae;
    }
};

struct AuthResponse {
    int status;
    std::vector<AuthSuccess> successes;
    std::vector<AuthError> errors;

    static AuthResponse fromJson(const JsonVariantConst json, const int status = 200) {
        AuthResponse res;
        res.status = status;
        if (json.is<JsonArrayConst>()) {
            const auto arr = json.as<JsonArrayConst>();
            for (JsonVariantConst v : arr) {
                if (v["success"].is<JsonVariantConst>()) {
                    res.successes.push_back(AuthSuccess::fromJson(v));
                } else if (v["error"].is<JsonVariantConst>()) {
                    res.errors.push_back(AuthError::fromJson(v));
                }
            }
        }
        return res;
    }
};

} // namespace Hue

#endif // LIGHTCONTROLLER_HUEMODELS_H
