
#include "uart_linux.h" // Header for UART functions

#define _GNU_SOURCE // Ensure termios exposes everything
// C standard libraries like glibc don't expose everything by default to avoid polluting the namespace, features like CRTSCTS are hidden unless you explicitly ask for them.
// #define _GNU_SOURCE → Enables __USE_MISC automatically( __USE_MISC 1)

#include <termios.h> // termios structure and UART config
#include <fcntl.h>	 // open()
#include <unistd.h>	 // read(), write(), close()
#include <stdio.h>	 // perror(), printf()

int uart_open(const char *device_path)
{
	// Open a UART Port
	// O_RDWR: Open for reading and writing
	// O_NOCTTY: This port should not become the controlling terminal for the process
	// O_SYNC: Writes are synchronized (block until completed)
	printf("\t⌛ Opening UART device path [%s]...\n", device_path);
	int fd = open(device_path, O_RDWR | O_NOCTTY | O_SYNC);
	printf("UART open() returned: %d\n", fd); // Add this line

	if (fd < 0)
	{
		fprintf(stderr, "\n\t❌ Failed to open UART device path [%s] (fd: %d)\n", device_path, fd);
		perror("\t📢 Error Msg");
		return -1;
	}
	// Get the Current UART Settings
	// tcgetattr() gets current terminal (UART) settings into the tty struct so we can modify them.
	struct termios tty;
	if (tcgetattr(fd, &tty) != 0)
	{
		perror("\t❌ Error Msg: tcgetattr");
		close(fd);
		return -1;
	}

	// Set Baud Rate
	// This sets both input and output baud rates to 115200 bps, which is common for embedded UART.
	cfsetospeed(&tty, B115200);
	cfsetispeed(&tty, B115200);

	// Configuring UART Flags
	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // CS8 sets character size to 8 bits
	tty.c_iflag &= ~IGNBRK;											// IGNBRK off → don’t ignore break signals
	tty.c_lflag = 0;														// c_lflag off → no line editing or signal interpretation(raw mode)
	tty.c_oflag = 0;														// c_oflag off → no output translation or delays

	// Wait until at least 1 byte is available OR timeout after 1.0 second. This is good for a blocking read with timeout fallback.
	tty.c_cc[VMIN] = 1;		// block until at least 1 byte is available
	tty.c_cc[VTIME] = 10; // or until 1 second passes

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // This disables "software flow control".
	// IXON = Enables XON (pause/resume) characters
	// IXOFF = Enables XOFF character
	// IXANY = Accept any character to resume output
	// 🔧 We **disable** all of them here, meaning: "Just send and receive bytes. No software-based pause/resume."
	tty.c_cflag |= (CLOCAL | CREAD);
	// CLOCAL = Ignore modem control lines (e.g., DCD, DSR). You're saying, "Just work like a local device."
	// CREAD = Enable the receiver — if you forget this, you can’t receive anything.
	// ✔️ Together: “Ignore modem lines, and enable receiving bytes.”
	tty.c_cflag &= ~(PARENB | PARODD); // no parity
	// PARENB = Enable parity generation and checking
	// PARODD = Use **odd** parity instead of even (if parity is enabled)
	// 🛑 We're disable both, which means: No parity bit at all — the ‘N’ in 8N1
	tty.c_cflag &= ~CSTOPB; // one stop bit
	// CSTOPB = If set, use 2 stop bits instead of 1.
	// We're clearing it, which means: Use 1 stop bit — the ‘1’ in 8N1

	tty.c_cflag &= ~CRTSCTS; // no hardware flow control
	// `CRTSCTS` = Enable **hardware flow control** (RTS/CTS)
	// We're disabling it, so: No hardware flow control (just raw RX/TX lines)

	/*
				## 🔚 Summary of What This Configures

	| Setting       | Meaning                             |
	|---------------|-------------------------------------|
	| `CS8`         | 8 data bits                         |
	| No `PARENB`   | No parity                           |
	| No `CSTOPB`   | 1 stop bit                          |
	| No `CRTSCTS`  | No hardware flow control            |
	| No `IXON/IXOFF` | No software flow control          |
	| `CREAD`       | Enable receiving                    |
	| `CLOCAL`      | Ignore modem control lines          |

	Together, this configures the UART for **standard 8N1 mode**:
	**8 data bits, no parity, 1 stop bit**, with **no flow control**.
	*/

	// Apply the Configuration
	// tcsetattr() applies the settings immediately (TCSANOW).
	if (tcsetattr(fd, TCSANOW, &tty) != 0)
	{
		perror("tcsetattr");
		close(fd);
		return -1;
	}

	// Return the File Descriptor
	return fd;
}
int uart_close(int *fd_ptr)
{
	// Close the UART Port
	if (close(*fd_ptr) < 0)
	{
		printf("\n\t❌ Failed to close UART(fd: %d)\n", *fd_ptr);
		perror("\t📢 Error Msg");
		return -1;
	}
	*fd_ptr = -1; // Set fd to -1 to indicate it's closed
	printf("\t✅ UART device closed successfully!\n");
	return 0;
}

// Read and Write Functions
int uart_read(int *fd_ptr, void *buf, int len)
{
	return read(*fd_ptr, buf, len);
}

int uart_write(int *fd_ptr, const void *buf, int len)
{
	return write(*fd_ptr, buf, len);
}
