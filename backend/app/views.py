"""Application routes for the Home Automation backend API."""

from math import floor
from os import getcwd
from os.path import exists, join
from time import time

from app import Config, Mqtt, app, mongo
from flask import jsonify, request, send_from_directory
from werkzeug.utils import secure_filename


# A. /api/set/combination - Save passcode from frontend
@app.route("/api/set/combination", methods=["POST"])
def set_combination():
    """Insert/update a single passcode document in the `code` collection."""
    try:
        passcode = request.form.get("passcode", "").strip()
        
        # Validate passcode is 4-digit number
        if not passcode.isdigit() or len(passcode) != 4:
            print(f"Invalid passcode format: {passcode}")
            return jsonify({"status": "failed", "data": "failed"})

        if mongo.set_passcode(passcode):
            print(f"Passcode saved successfully: {passcode}")
            return jsonify({"status": "complete", "data": "complete"})
        
        print("Failed to save passcode to database")
        return jsonify({"status": "failed", "data": "failed"})
    except Exception as error:
        print(f"Error in set_combination: {error}")
        return jsonify({"status": "failed", "data": "failed"})


# B. /api/check/combination - Validate passcode from hardware (ESP32)
@app.route("/api/check/combination", methods=["POST"])
def check_combination():
    """Validate a 4-digit passcode against the stored passcode."""
    try:
        passcode = request.form.get("passcode", "").strip()
        
        # Validate passcode is 4-digit number
        if not passcode.isdigit() or len(passcode) != 4:
            print(f"Invalid passcode format: {passcode}")
            return jsonify({"status": "failed", "data": "failed"})

        # Check if passcode exists in database
        if mongo.count_passcode(passcode) > 0:
            print(f"Valid passcode: {passcode}")
            return jsonify({"status": "complete", "data": "complete"})
        
        print(f"Invalid passcode: {passcode}")
        return jsonify({"status": "failed", "data": "failed"})
    except Exception as error:
        print(f"Error in check_combination: {error}")
        return jsonify({"status": "failed", "data": "failed"})


# C. /api/update - Receive sensor data from hardware (Arduino Nano)
@app.route("/api/update", methods=["POST"])
def update():
    """Store hardware readings and publish the same payload on MQTT."""
    try:
        payload = request.get_json(silent=True)
        if not isinstance(payload, dict):
            print("No JSON data provided or invalid format")
            return jsonify({"status": "failed", "data": "failed"})

        print(f"Received data from hardware: {payload}")

        # Validate required fields
        required_fields = ["id", "type", "radar", "waterheight", "reserve", "percentage"]
        for field in required_fields:
            if field not in payload:
                print(f"Missing required field: {field}")
                return jsonify({"status": "failed", "data": "failed"})

        # Add 10-digit timestamp (Unix timestamp)
        current_timestamp = floor(time())
        payload["timestamp"] = current_timestamp
        print(f"Added timestamp: {current_timestamp}")

        # Publish to MQTT topic for frontend subscription
        topic = str(payload["id"])
        published = Mqtt.publish(topic, mongo.dumps(payload))
        print(f"Published to MQTT topic '{topic}': {published}")

        # Insert into radar collection
        saved = mongo.insert_radar(payload)
        print(f"Data inserted into radar collection: {saved}")

        if published and saved:
            print("Update successful - data published and saved")
            return jsonify({"status": "complete", "data": "complete"})
        
        print(f"Update partial failure - published: {published}, saved: {saved}")
        return jsonify({"status": "failed", "data": "failed"})
    except Exception as error:
        print(f"Error in update: {error}")
        return jsonify({"status": "failed", "data": "failed"})


# D. /api/reserve/<start>/<end> - Get radar data between timestamps
@app.route("/api/reserve/<start>/<end>", methods=["GET"])
def reserve(start, end):
    """Return all radar records between start and end Unix timestamps."""
    try:
        # Convert to integers
        start_ts = int(start)
        end_ts = int(end)

        # Validate timestamp range
        if end_ts < start_ts:
            print(f"Invalid range: start={start_ts} > end={end_ts}")
            return jsonify({"status": "failed", "data": 0})

        print(f"Fetching radar data between {start_ts} and {end_ts}")
        
        # Get data from database
        data = mongo.get_radar_between(start_ts, end_ts)
        
        print(f"Found {len(data)} records in date range")
        return jsonify({"status": "found", "data": data})
    except ValueError as ve:
        print(f"Invalid timestamp format: start={start}, end={end} - {ve}")
        return jsonify({"status": "failed", "data": 0})
    except Exception as error:
        print(f"Error in reserve: {error}")
        return jsonify({"status": "failed", "data": 0})


# E. /api/avg/<start>/<end> - Get average of reserve field between timestamps
@app.route("/api/avg/<start>/<end>", methods=["GET"])
def avg(start, end):
    """Return arithmetic average of `reserve` between two timestamps."""
    try:
        # Convert to integers
        start_ts = int(start)
        end_ts = int(end)

        # Validate timestamp range
        if end_ts < start_ts:
            print(f"Invalid range: start={start_ts} > end={end_ts}")
            return jsonify({"status": "failed", "data": 0})

        print(f"Calculating average reserve between {start_ts} and {end_ts}")

        # Get average from database using aggregation
        result = mongo.get_reserve_average_between(start_ts, end_ts)
        
        # Extract average from result
        if result and len(result) > 0:
            average = result[0].get("average", 0)
            # Round to 2 decimal places for display
            average = round(average, 2)
            print(f"Calculated average reserve: {average} gallons")
            return jsonify({"status": "found", "data": average})
        
        print("No data found for average calculation")
        return jsonify({"status": "found", "data": 0})
    except ValueError as ve:
        print(f"Invalid timestamp format: start={start}, end={end} - {ve}")
        return jsonify({"status": "failed", "data": 0})
    except Exception as error:
        print(f"Error in avg: {error}")
        return jsonify({"status": "failed", "data": 0})


# F. Health check endpoint
@app.route("/api/health", methods=["GET"])
def health_check():
    """Simple health check endpoint to verify API is running."""
    return jsonify({
        "status": "ok", 
        "message": "Backend API is running",
        "timestamp": floor(time())
    })


# G. File serving endpoints (existing)
@app.route("/api/file/get/<filename>", methods=["GET"])
def get_images(filename):
    """Return requested file from uploads folder."""
    directory = join(getcwd(), Config.UPLOADS_FOLDER)
    file_path = join(getcwd(), Config.UPLOADS_FOLDER, filename)

    if exists(file_path):
        return send_from_directory(directory, filename)
    return jsonify({"status": "file not found"}), 404


# H. File upload endpoint (existing)
@app.route("/api/file/upload", methods=["POST"])
def upload():
    """Save a file to the uploads folder."""
    file = request.files["file"]
    filename = secure_filename(file.filename)
    file.save(join(getcwd(), Config.UPLOADS_FOLDER, filename))
    return jsonify({"status": "File upload successful", "filename": filename})


# I. Optional: Get current passcode (for debugging)
@app.route("/api/get/currentcode", methods=["GET"])
def get_current_code():
    """Helper endpoint to retrieve current passcode (for testing only)."""
    try:
        # This is just for debugging - uses existing mongo connection
        client = mongo._client()
        collection = client["ELET2415"]["code"]
        result = collection.find_one({"type": "passcode"}, {"_id": 0})
        if result:
            return jsonify({"status": "found", "data": result})
        return jsonify({"status": "not found", "data": None})
    except Exception as error:
        print(f"Error in get_current_code: {error}")
        return jsonify({"status": "failed", "data": None})
    finally:
        if client:
            client.close()


# J. Optional: Get recent radar data (for testing)
@app.route("/api/recent/<int:limit>", methods=["GET"])
def get_recent(limit=10):
    """Helper endpoint to get most recent radar data (for testing)."""
    try:
        client = mongo._client()
        collection = client["ELET2415"]["radar"]
        cursor = collection.find({}, {"_id": 0}).sort("timestamp", -1).limit(limit)
        data = list(cursor)
        return jsonify({"status": "found", "data": data})
    except Exception as error:
        print(f"Error in get_recent: {error}")
        return jsonify({"status": "failed", "data": []})
    finally:
        if client:
            client.close()


# Response headers (existing)
@app.after_request
def add_header(response):
    """Force no-cache behavior for browser refresh consistency."""
    response.headers["X-UA-Compatible"] = "IE=Edge,chrome=1"
    response.headers["Cache-Control"] = "public, max-age=0"
    return response


# Error handler (existing)
@app.errorhandler(405)
def page_not_found(error):
    """Return JSON for unsupported methods."""
    return jsonify({"status": 404}), 404
