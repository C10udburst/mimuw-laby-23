import os
import subprocess
import ppcb
import time


def compile(packet_size):
    print(f"Compiling with packet size {packet_size}...")
    os.system(f"make clean")
    os.system(f"make docker DATA_SIZE={packet_size}")
    
def run(protocol):
    server_prot = protocol if protocol != "udpr" else "udp"
    print(f"Running with protocol {protocol}...")
    server = subprocess.Popen(["./run", "server", "bash", "-c", f"timeout 300s /work/ppcbs {server_prot} 21370 > /dev/null"], stdin=subprocess.PIPE, stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
    client = subprocess.Popen(["./run", "client1", "bash", "-c", f"cat /work/bigfile.bin | timeout 300s /work/ppcbc {protocol} server 21370"], stdin=subprocess.PIPE, stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
    try:
        client.wait()
    except:
        pass
    time.sleep(1)
    server.terminate()
    
def env(tc = None):
    print(f"Setting up environment with tc {tc}...")
    if tc is not None:
        os.system(f"./tc/{tc}.sh")
        os.system(f"./tc/query.sh")
        
def collect(packet_size, protocol, tc = None, id = 0):
    tc = tc or "none"
    os.system(f"mkdir -p results/{packet_size}/{protocol}/{tc}/{id}")
    os.system(f"./run server cp /tmp/server.ppcb /work/results/{packet_size}/{protocol}/{tc}/{id}/server.ppcb")
    os.system(f"./run client1 cp /tmp/client.ppcb /work/results/{packet_size}/{protocol}/{tc}/{id}/client.ppcb")
    
start_size = 48
end_size = 1514
steps = 8
packet_sizes = [start_size + i * (end_size - start_size) // steps for i in range(steps + 1)]

print(packet_sizes)

tc_types = [None, "delay", "loss", "reorder", "dup"]
    
def validate(packet_size, protocol, tc, id):
    tc = tc or "none"
    print(f"Validating with packet size {packet_size}, protocol {protocol} and tc {tc}...")
    # get last packet from iterator
    try:
        last_packet = None
        for packet in ppcb.read(f"results/{packet_size}/{protocol}/{tc}/{id}/server.ppcb"):
            last_packet = packet
        print(last_packet)
    except Exception as e:
        open(f"results/{packet_size}/{protocol}/{tc}/{id}/error.txt", "w").write(str(e))
        print(e)
        
total_count = len(packet_sizes) * 3 * len(tc_types)
print(f"Total count: {total_count}")
count = 0
for packet_size in packet_sizes:
    for protocol in ["tcp", "udp", "udpr"]:
        for tc in tc_types:
            for i in range(3):
                compile(packet_size)
                env(tc)
                run(protocol)
                collect(packet_size, protocol, tc, i)
                validate(packet_size, protocol, tc, i)
                count += 1
                print(f"\033[92m Progress: {count}/{total_count} \033[0m")
                if (count % 20 == 0):
                    os.system("docker system prune -f")