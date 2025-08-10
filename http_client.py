import requests

BASE_URL = "http://localhost:8080"

def login(username, password):
    print("[*] Logging in...")
    data = {
        "username": username,
        "password": password
    }
    response = requests.post(f"{BASE_URL}/api/login", data=data)
    
    if response.status_code == 200:
        session_id = response.text.strip()
        print(f"[+] Login successful. Session ID: {session_id}")
        return session_id
    else:
        print(f"[-] Login failed: {response.status_code}")
        print(response.text)
        return None

def run_command(session_id, command):
    print(f"[*] Running command: {command}")
    data = {
        "session_id": session_id,
        "command": command
    }
    response = requests.post(f"{BASE_URL}/api/run", data=data)
    
    if response.status_code == 200:
        print("[+] Command output:")
        print(response.text)
    else:
        print(f"[-] Command failed: {response.status_code}")
        print(response.text)

if __name__ == "__main__":
    # Change credentials and command as needed
    session = login("user", "XXX")
    session = login("peter", "12345")
    if session:
        run_command(session, "uname -a")
        run_command(session, "ps faux")
