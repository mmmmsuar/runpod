import subprocess
import runpod
import os

def worker_function(job):
    start_key_range = job['input']['start_key_range']
    end_key_range = job['input']['end_key_range']
    
    command = f"./keygenc {start_key_range} {end_key_range}"
    try:
        result = subprocess.run(command, shell=True, capture_output=True, text=True, cwd="/app")
        if result.returncode == 0:
            sync_result = sync_data_to_local()
            if sync_result["status"] == "error":
                return sync_result
            sync_result = sync_local_to_drive()
            if sync_result["status"] == "error":
                return sync_result
            cleanup_result = cleanup_files()
            if cleanup_result["status"] == "error":
                return cleanup_result
            return {"status": "completed", "output": result.stdout, "stderr": result.stderr}
        else:
            return {"status": "error", "message": result.stderr}
    except subprocess.CalledProcessError as e:
        return {"status": "error", "message": str(e)}

def sync_data_to_local():
    try:
        rclone_command = [
            "rclone", "sync",
            "/app/output",
            "local:C:\\Development\\Puzzle\\Generated",
            "--config", "/app/rclone.conf"
        ]
        subprocess.run(rclone_command, check=True)
        print("Sync to local completed successfully.")
        return {"status": "success"}
    except subprocess.CalledProcessError as e:
        return {"status": "error", "message": f"Sync to local failed: {str(e)}"}

def sync_local_to_drive():
    try:
        rclone_command = [
            "rclone", "sync",
            "local:C:\\Development\\Puzzle\\Generated",
            "gdrive:/folders/174j3DSKbN8lbZ4MrbGaiZLWmzj6EuMUj",
            "--config", "/app/rclone.conf"
        ]
        subprocess.run(rclone_command, check=True)
        print("Sync to Google Drive completed successfully.")
        return {"status": "success"}
    except subprocess.CalledProcessError as e:
        return {"status": "error", "message": f"Sync to Google Drive failed: {str(e)}"}

def cleanup_files():
    try:
        for file in os.listdir("/app/output"):
            os.remove(os.path.join("/app/output", file))
        print("Cleanup completed successfully.")
        return {"status": "success"}
    except Exception as e:
        return {"status": "error", "message": f"Cleanup failed: {str(e)}"}

runpod.serverless.start({"handler": worker_function})