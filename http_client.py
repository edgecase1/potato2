import requests

BASE_URL = "http://localhost"

class PotatoClient:

    def login(self, username, password):
        self.s = requests.Session()

        print("[*] Logging in...")
        data = {
            "username": username,
            "password": password
        }

        response = self.s.post(f"{BASE_URL}/api/login", data=data)
        
        if response.status_code == 200:
            print(f"[+] Login successful.")
            print(f"session value is {self.s.cookies['session']}")
            return True
        else:
            print(f"[-] Login failed: {response.status_code}")
            print(response.text)
            return False
    
    def run_command(self, command):
        print(f"[*] Running command: {command}")
        data = {
            "command": command
        }
        response = self.s.post(f"{BASE_URL}/api/run", data=data)
        
        if response.status_code == 200:
            print("[+] Command output:")
            print(response.text)
        else:
            print(f"[-] Command failed: {response.status_code}")
            print(response.text)
    
if __name__ == "__main__":
    # Change credentials and command as needed
    #session = login("user", "XXX")
    c = PotatoClient()
    if c.login("peter", "12345"):
        c.run_command("uname -a")
        c.run_command("id")
        #c.run_command("ps faux")
        #c.run_command("ls -la /")
