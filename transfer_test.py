#!/usr/bin/python3

from mininet.topo import Topo
from mininet.net import Mininet
from mininet.node import CPULimitedHost
from mininet.link import TCLink
from mininet.util import dumpNodeConnections
from mininet.log import setLogLevel

from sys import argv, exit
from time import sleep
import os.path

class SingleSwitchTopo(Topo):
    """
    Class representing a network with a single switch connected to n hosts.
    This switch has a max_queue_size of 2, forcing a stop-and-wait approach to sending.
    """
    def build(self, n=2, ms_delay=10, loss_rate=5):
        switch = self.addSwitch('s1')
        for h in range(n):
            # Each host gets 50%/n of system CPU
            host = self.addHost('h%s' % (h + 1), cpu=.5/n)

            # 10 Mbps, 2 packet queue; delay and loss rate are parameters
            self.addLink(host, switch, bw=10, delay='%dms' % (ms_delay), loss=loss_rate,
                          max_queue_size=2, use_htb=True)

def run_test(delay=10, loss=5):
    """
    Runs the sender and receiver to transfer 1000lines.txt over the simulated network.

    Parameters:
    delay (int): The delay (in ms) to transfer across one line in the network.
    loss (int): The loss rate for each link in the network.
    """

    # Create network topology that creates 2 hosts separated by a single switch
    topo = SingleSwitchTopo(n=2, ms_delay=delay, loss_rate=loss)
    net = Mininet(topo=topo, host=CPULimitedHost, link=TCLink)
    net.start()

    h1, h2 = net.get( 'h1', 'h2' )

    # Try creating the test directory.
    h1.cmd("mkdir test")

    # remove old test files (if there are any in there currently)
    h1.cmd("rm -f test/*")

    # Have h2 run the receiver and store the received data in test/received-data.txt
    print("Starting receiver on h2, port 2000... saving data to test/received-data.txt")
    h2.cmd('timeout 10s ./receiver 2000 > test/received-data.txt 2> test/receiver-output.err.txt &')

    # Sleep for a short time (0.5 seconds) to allow the receiver to start running
    sleep(0.5)

    # Have h1 run the sender to transfer the file
    print("Starting sender on h1...")
    h1.cmd(f"timeout 10s ./sender {h2.IP()} 2000 < 1000lines.txt > test/sender-output.txt 2> test/sender-output.err.txt")

    # check to see if either sender (h1) or receiver (h2) timed out
    h1_exit_status = h1.cmd("echo $?")
    h2_exit_status = h2.cmd("echo $?")

    # Note: 124 is the value returned by the timeout program if there was a timeout
    if h1_exit_status.strip() == "124":
        print("ERROR: Sender timed out after 10 seconds.")
    if h2_exit_status.strip() == "124":
        print("ERROR: Receiver timed out after 10 seconds.")

    if not os.path.isfile("test/received-data.txt"):
        print("ERROR: Couldn't find the file test/received-data.txt")
    else:
        print("\nComparing md5sum of original and received file.")
        original_md5 = h1.cmd("md5sum 1000lines.txt").split()[0]
        received_md5 = h1.cmd("md5sum test/received-data.txt").split()[0]
        print(f"\tOriginal: {original_md5}")
        print(f"\tReceived: {received_md5}")
        if original_md5 == received_md5:
            print("\n\tSUCCESS: md5sums are the same!")
        else:
            print("\n\tFAILED: md5sums did not match!")

    net.stop()


if __name__ == '__main__':
    if len(argv) != 3:
        exit("Usage: %s delay loss_rate" % argv[0])

    print("Delay (ms): ", argv[1])
    print("Loss Rate: ", argv[2])

    setLogLevel( 'info' )
    run_test(delay=int(argv[1]), loss=int(argv[2]))
