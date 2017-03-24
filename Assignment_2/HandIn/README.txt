Isaac Sahle V00816592	B08

My design is based off the sliding window protocol variation (Go back N).
The main principle is...

Connection Establishment:
	- Sender sends initial SYN with with randomly chosen sequence number and waits a small amount of time for ACK
	- Receiver recieves SYN and replies with ACK.
	- Sender retransmits the SYN if no ACK is received within a specific amount of time.

Data Transfer:
	- Sender sends a group of DAT packets to fill window; pending packets are bufferd with a timestamp
	- Receiver sends ACKs as the packets arrive
	- Sender slides window as ACKs are received
	- Any buffered packets with expired timestamps are retransmitted

Connection Teardown:
	- Sender sends a FIN and waits for ACK
	- Receiver receives FIN, sends back ACK, and enters time wait in which the receiver should not receive another FIN
	- Sender receives ACK and closes
	- If receiver did not receive another FIN within the time period then receiver closes
