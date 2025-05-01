from flask import Flask, request, jsonify
import random

app = Flask(__name__)

# 🔢 Number of cubes you're controlling
NUM_CUBES = 100  # change as needed

# 🔁 Generate random movement commands (yaw always 0.0)
def generate_random_move_commands():
    return {
        cube_id: {
            "x": random.uniform(1.0, -1.0),
            "y": random.uniform(1.0, 1.0),
            "yaw": 0.0
        }
        for cube_id in range(NUM_CUBES)
    }

@app.route('/receive_data', methods=['POST'])
def receive_data():
    data = request.get_json()
    if not data:
        return jsonify({'status': 'error', 'message': 'No JSON received'}), 400

    print("=== Received Cube Data ===")
    # cubes = data.get("cubes", [])
    # for cube in cubes:
    #     cube_id = cube.get("id", -1)
    #     player_loc = cube.get("Player_Location", {})
    #     win_loc = cube.get("win_location", {})
    #     scans = cube.get("scan_results", [])

    #     print(f"Cube ID: {cube_id}")
    #     print(f"  Player Location: ({player_loc.get('x')}, {player_loc.get('y')}, {player_loc.get('z')})")
    #     print(f"  Win Location: ({win_loc.get('x')}, {win_loc.get('y')}, {win_loc.get('z')})")

    #     print("  Scan Results:")
    #     for scan in scans:
    #         print(f"    → Hit @ ({scan.get('x')}, {scan.get('y')}, {scan.get('z')}), Actor: {scan.get('actor')}")
    #     print("")
    cubes = data.get("cubes", [])
    for cube in cubes:
        cube_id = cube.get("id", -1)
        status = cube.get("status", "unknown")
        print(f"Cube {cube_id}: {status}")
    # ✅ Generate and return movement commands for all cubes
    move_commands = generate_random_move_commands()
    all_commands = [
        {"id": cube_id, **cmd}
        for cube_id, cmd in move_commands.items()
    ]
    return jsonify({"commands": all_commands})


@app.route('/get_move')
def get_move():
    move_commands = generate_random_move_commands()
    all_commands = [
        {"id": cube_id, **cmd}
        for cube_id, cmd in move_commands.items()
    ]
    return jsonify({"commands": all_commands})


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8000, threaded=False)
