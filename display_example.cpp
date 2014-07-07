/**
    Example code to demonstrate the usage of the CX2100 power supply display

    Some ASCII control characters are used to control the display behavior:
    '\021': switch the backlight on
    '\023': switch the backlight off
    '\f': clear screen and move cursor to the first charachter of the first row
    '\b': move cursor to previous character
    '\n': move cursor to start of next row
    '\r': move cursor to first character in row
    '\t': move cursor to next character

    Copyright (C) 2014  Beckhoff Automation GmbH
    Author: Patrick Bruenn <p.bruenn@beckhoff.com>

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files
    (the "Software"), to deal in the Software without restriction,
    including without limitation the rights to use, copy, modify, merge,
    publish, distribute, sublicense, and/or sell copies of the Software,
    and to permit persons to whom the Software is furnished to do so,
    subject to the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define DEVICE_NAME "/dev/cx_display"

/** for delay and multithreading only */
#include <chrono>
#include <thread>
#include <vector>

void backlight_example()
{
	static const char off[] = "\023\fbacklight is off";
	static const char on[] = "\021backlight is on\n";
	const int fd = open(DEVICE_NAME, O_WRONLY);
	if (-1 == fd) {
		perror(NULL);
		return;
	}
	
	write(fd, off, sizeof(off));
	std::this_thread::sleep_for(std::chrono::seconds(2));
	write(fd, on, sizeof(on));
	std::this_thread::sleep_for(std::chrono::seconds(2));
	close(fd);
}

void simple_example()
{
	const int fd = open(DEVICE_NAME, O_WRONLY);
	if (-1 == fd) {
		perror(NULL);
		return;
	}

	static const char first[] = "\fHello world!";
	write(fd, first, sizeof(first));
	std::this_thread::sleep_for(std::chrono::seconds(2));
	static const char second[] = "\fI am alive!\nand duallined!";
	write(fd, second, sizeof(second));
	std::this_thread::sleep_for(std::chrono::seconds(2));
	close(fd);
}

void complex_example()
{
	const int fd = open(DEVICE_NAME, O_WRONLY);
	if (-1 == fd) {
		perror(NULL);
		return;
	}

	static const char clearscreen[] = "\fHello there?";
	write(fd, clearscreen, sizeof(clearscreen));
	std::this_thread::sleep_for(std::chrono::seconds(2));

	static const char overwrite_char[] = "\b\b\b\b\b\bworld!";	
	write(fd, overwrite_char, sizeof(overwrite_char));
	std::this_thread::sleep_for(std::chrono::seconds(2));

	static const char append[] = "\nI am alive?";
	write(fd, append, sizeof(append));
	std::this_thread::sleep_for(std::chrono::seconds(2));

	static const char overwrite_line[] = "\rIt's\t\t\t\t\t\t!";
	write(fd, overwrite_line, sizeof(overwrite_line));
	std::this_thread::sleep_for(std::chrono::seconds(2));
	close(fd);
}

void RunAtPos(size_t pos)
{
	const int fd = open(DEVICE_NAME, O_WRONLY);
	if (-1 == fd) {
		perror(NULL);
		return;
	}

	/** move cursor to our position */
	for (size_t i = 0; i <= pos; ++i) {
		static const char tab = '\t';
		write(fd, &tab, 1);
	}

	/** print some characters */
	for (unsigned char c = '~'; c != ('/' + pos % 10); c--) {
		const unsigned char replace_char[2] = {'\b', c};
		write(fd, replace_char, sizeof(replace_char));
		/** sleep a little to let the other threads work, too */
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
	close(fd);
}

void multithreaded_example()
{
	std::vector<std::thread> threads(32);
	for (size_t i = 0; i < threads.size(); ++i) {
		threads[i] = std::thread(&RunAtPos, i);
	}
		
	for (auto& t: threads) {
		t.join();
	}
}

int main(int argc, char *argv[])
{
	/**
	 * use ASCII code DC1(XON) and DC3(XOFF) to control the backlight
	 */
	backlight_example();
	
	/**
	 * Only one thread accesses the display
	 */
	simple_example();
	complex_example();

	/**
	 * Multiple threads writing simultanously to different areas of the display
	 */
	multithreaded_example();
	return 0;
}
