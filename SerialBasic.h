#ifndef SERIAL_BASIC_H_
#define SERIAL_BASIC_H_

#include <string>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <cstdint>
#include <list>
#include <sstream>
#include <iterator>
#include <memory>

/**
 * @file SerialBasic.h
 * @author  Andrew Powell <andrew.powell@temple.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * Copyright (C) 2014  Andrew Powell
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

template <class Type = uint8_t> class SerialBasic;
typedef SerialBasic<> Serial;

/**
 * \brief Writes and reads data to a serial port with 8N1, for a particular Type
 *
 * Since SerialBasic was originally developed for communication with the Xbee transcievers, SerialBasic is generalized 
 * such that different data types can be serially transmitted. The default type of uint8_t is introduced with the 
 * assumption most developers will only transmit uint8_t arrays or other containers (e.g. vectors, lists, etc.). 
 *
 * Please note SerialBasic is developed with boost 1.55 and depends on boost threads (thus, ensure the boost compiled
 * binaries are linked with the application utilizing SerialBasic). SerialBasic also depends on lambda functions
 * from the C++0x standard. SerialBasic is also intended for Windows operating systems, however the constructor method
 * can be modified for other operating systems supported by boost.
 *
 * @see www.boost.org
 */
template <class Type>
class SerialBasic {
public:
	typedef uint8_t Byte;

	/**
	 * \brief Attempt to open serial port with a given comPort and baudRate
	 *
	 * @param comPort The COM port
	 * @param baudRate The baudrate
	 * @throw boost::system::system_error Thrown if the attempt to open the serial port failed. Check boost error code
	 * to find out the reason of the failure.
	 */
	SerialBasic(uint16_t comPort, uint32_t baudRate);

	/**
	 * \brief Destroy SerialBasic object
	 */
	~SerialBasic();

	/**
	 * \brief Get boost error code
	 *
	 * The error code can be used to verify whether a previously established connection is lost and why.
	 *
	 * @return The boost error code.
	 */
	boost::system::error_code& getErrorCode();

	/**
	 * \brief Read serial data from the SerialBasic object's buffer (non-blocking)
	 *
	 * @param beginIterator The starting location of where the data is saved. Can be a pointer to an array or an iterator 
	 * of a container.
	 * @param size The maximum amount of data to copy from the SerialBasic object's buffer. For instance, if beginIterator 
	 * is a pointer to an array of characters, size is the maxmimum amount of characters copied from the SerialBasic object's 
	 * buffer. It is up to the developer to ensure the buffer to which beginIterator refers has the capacity to contain the 
	 * maximum amount of requested data.
	 * @return The actual amount of data saved.
	 */
	template <class BeginIterator>
	std::size_t read(BeginIterator beginIterator, std::size_t size);

	/**
	 * \brief Write serial data to the serial port (blocking)
	 *
	 * @param beginIterator The starting location of where the data is taken. Can be a pointer to an array or an iterator 
	 * of a container.
	 * @param size The maximum amount of data to write to the serial port. For instance, if beginIterator is a pointer to an
	 * array of characters, size is the maximum amount of characters written to the serial port associated with the SerialBasic
	 * object. It is up to the developer to ensure the buffer to which beginIterator refers is at least as large as the 
	 * specified size.
	 * @throw boost::system::system_error Thrown if there is a failure to write data to the serial port. Check boost error code
	 * to find out the reason of the failure.
	 */
	template <class BeginIterator>
	void write(BeginIterator beginIterator, std::size_t size);
private:
	boost::recursive_mutex recursiveMutex;
	boost::system::error_code errorCode;
	const static std::size_t READ_BUFFER_SIZE = 512;
	const static std::size_t READ_TRANSFER_BUFFER_SIZE = 128;
	Byte readTransferBuffer[READ_TRANSFER_BUFFER_SIZE];
	std::list<Byte> readBuffer;
	boost::asio::io_service io;
	boost::asio::io_service::work work_;
	boost::asio::strand strand_;
	boost::asio::serial_port serial;
	boost::thread thread_;
	void setAsynchronousRead() {
		serial.async_read_some(
				boost::asio::buffer(readTransferBuffer, READ_TRANSFER_BUFFER_SIZE),
				strand_.wrap([&](const boost::system::error_code& error, std::size_t size)->void{
			boost::unique_lock<boost::recursive_mutex> scoped_lock(recursiveMutex);
			if (error == false) {
				std::size_t bytesSum = readBuffer.size()+size;
				std::size_t bytesRemaining =  (bytesSum < READ_BUFFER_SIZE) ? READ_BUFFER_SIZE-bytesSum : 0;
				std::size_t bytesToTransfer = (bytesRemaining < size) ? bytesRemaining : size;
				std::copy(readTransferBuffer, readTransferBuffer+bytesToTransfer, std::back_inserter(readBuffer));
				setAsynchronousRead();
			}
			errorCode = error;
		}));
	}
};

template <class Type>
SerialBasic<Type>::SerialBasic(uint16_t comPort, uint32_t baudRate) : work_(io), strand_(io), serial(io) {

		// attempt to open com port
		std::stringstream ss;
		ss << "COM" << comPort;
		serial.open(ss.str());

		// set options
		serial.set_option(boost::asio::serial_port_base::parity());	
		serial.set_option(boost::asio::serial_port_base::character_size(8));
		serial.set_option(boost::asio::serial_port_base::stop_bits());	
		serial.set_option(boost::asio::serial_port_base::baud_rate(baudRate));

		// set up thread for io service
		thread_ = boost::thread([&]()->void{
			try {
				io.run();
			} catch (boost::system::system_error& e) {
				boost::unique_lock<boost::recursive_mutex> scoped_lock(recursiveMutex);
				errorCode = e.code();
			}
		});

		// set asynchronous read
		setAsynchronousRead();
}

template <class Type>
SerialBasic<Type>::~SerialBasic() {
	io.stop();
	serial.close();
	thread_.join();
}

template <class Type>
boost::system::error_code& SerialBasic<Type>::getErrorCode() {
	boost::unique_lock<boost::recursive_mutex> scoped_lock(recursiveMutex);
	return errorCode;
}

template <class Type>
template <class BeginIterator>
std::size_t SerialBasic<Type>::read(BeginIterator beginIterator, std::size_t size) {
	boost::unique_lock<boost::recursive_mutex> scoped_lock(recursiveMutex);
	std::size_t numberOfCompletedItems = readBuffer.size()/sizeof(Type);
	std::size_t itemsToTransfer = (numberOfCompletedItems < size) ? 
		numberOfCompletedItems : 
		size;
	if (itemsToTransfer == 0)
		return 0;
	std::size_t bytesToTransfer = itemsToTransfer*sizeof(Type); 
	std::unique_ptr<Byte[]> buffer(new Byte[bytesToTransfer]);
	std::list<Byte>::iterator end = readBuffer.begin();
	std::advance(end, bytesToTransfer);
	std::copy(readBuffer.begin(), end, buffer.get());
	readBuffer.erase(readBuffer.begin(), end);
	std::copy((Type*)buffer.get(), 
		((Type*)buffer.get())+itemsToTransfer, 
		beginIterator);
	return itemsToTransfer;
}

template <class Type>
template <class BeginIterator>
void SerialBasic<Type>::write(BeginIterator beginIterator, std::size_t size) {
	std::unique_ptr<Type[]> buffer(new Type[size]);
	for (std::size_t i = 0; i < size; i++)
		buffer.get()[i] = *(beginIterator++);
	boost::asio::write(serial, boost::asio::buffer(buffer.get(), size*sizeof(Type)));
}

#endif