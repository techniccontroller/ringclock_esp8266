from zeroconf import Zeroconf, ServiceBrowser, ServiceListener
import time

class Listener(ServiceListener):
    def add_service(self, zc, type_, name):
        info = zc.get_service_info(type_, name)
        print("\nSERVICE:", type_)
        print("NAME:", name)
        if info:
            print("PORT:", info.port)
            print("SERVER:", info.server)
            print("ADDRESSES:", [".".join(map(str, a)) for a in info.addresses])
            print("PROPERTIES:", {k.decode(errors="ignore"): v.decode(errors="ignore") for k, v in info.properties.items()})

    def update_service(self, zc, type_, name):
        pass

    def remove_service(self, zc, type_, name):
        pass

zc = Zeroconf()
listener = Listener()

for service in [
    "_arduino._tcp.local.",
    "_esp8266._tcp.local.",
    "_http._tcp.local.",
    "_services._dns-sd._udp.local.",
]:
    ServiceBrowser(zc, service, listener)

print("Listening for 30 seconds...")
time.sleep(30)
zc.close()