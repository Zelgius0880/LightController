import asyncio
import websockets

async def listen_to_server():
    # Replace this with your actual WebSocket server URL
    uri = "ws://192.168.1.48:81"

    try:
        # Connect to the server
        async with websockets.connect(uri) as websocket:
            print(f"--- Successfully connected to {uri} ---")

            # Keep the connection open and listen for messages
            while True:
                try:
                    message = await websocket.recv()
                    print(f"Incoming message: {message}")
                except websockets.ConnectionClosed:
                    print("Connection closed by the server.")
                    break
    except Exception as e:
        print(f"Could not connect: {e}")

# Run the listener
if __name__ == "__main__":
    asyncio.run(listen_to_server())