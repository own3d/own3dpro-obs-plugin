// Integration of the OWN3D service into OBS Studio
// Copyright (C) 2020 own3d media GmbH <support@own3d.tv>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "curl.hpp"
#include <sstream>

int32_t own3d::util::curl::debug_helper(CURL* handle, curl_infotype type, char* data, size_t size, util::curl* self)
{
	if (self->_debug_callback) {
		self->_debug_callback(handle, type, data, size);
	} else {
#ifdef _DEBUG_CURL
		std::stringstream hd;
		for (size_t n = 0; n < size; n++) {
			hd << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << static_cast<int32_t>(data[n])
			   << " ";
			if (n % 16 == 15) {
				hd << "\n  ";
			}
		}
		std::string hds = hd.str();

		switch (type) {
		case CURLINFO_TEXT:
			DLOG_DEBUG("<CURL> %.*s", size - 1, data);
			break;
		case CURLINFO_HEADER_IN:
			DLOG_DEBUG("<CURL> << Header: %.*s", size - 1, data);
			break;
		case CURLINFO_HEADER_OUT:
			DLOG_DEBUG("<CURL> >> Header: %.*s", size - 1, data);
			break;
		case CURLINFO_DATA_IN:
			DLOG_DEBUG("<CURL> << %lld bytes of data:\n  %s", size, hds.c_str());
			break;
		case CURLINFO_DATA_OUT:
			DLOG_DEBUG("<CURL> >> %lld bytes of data:\n  %s", size, hds.c_str());
			break;
		case CURLINFO_SSL_DATA_IN:
			DLOG_DEBUG("<CURL> << %lld bytes of SSL data:\n  %s", size, hds.c_str());
			break;
		case CURLINFO_SSL_DATA_OUT:
			DLOG_DEBUG("<CURL> >> %lld bytes of SSL data:\n  %s", size, hds.c_str());
			break;
		}
#endif
	}
	return 0;
}

size_t own3d::util::curl::read_helper(void* ptr, size_t size, size_t count, util::curl* self)
{
	if (self->_read_callback) {
		return self->_read_callback(ptr, size, count);
	} else {
		return size * count;
	}
}

size_t own3d::util::curl::write_helper(void* ptr, size_t size, size_t count, util::curl* self)
{
	if (self->_write_callback) {
		return self->_write_callback(ptr, size, count);
	} else {
		return size * count;
	}
}

int32_t own3d::util::curl::xferinfo_callback(util::curl* self, curl_off_t dlt, curl_off_t dln, curl_off_t ult,
											 curl_off_t uln)
{
	if (self->_xferinfo_callback) {
		return self->_xferinfo_callback(static_cast<uint64_t>(dlt), static_cast<uint64_t>(dln),
										static_cast<uint64_t>(ult), static_cast<uint64_t>(uln));
	} else {
		return 0;
	}
}

own3d::util::curl::curl() : _curl(), _read_callback(), _write_callback(), _headers()
{
	_curl = curl_easy_init();
	set_read_callback(nullptr);
	set_write_callback(nullptr);
	set_xferinfo_callback(nullptr);
	set_debug_callback(nullptr);

	// Default settings.
	set_option(CURLOPT_NOPROGRESS, false);
	set_option(CURLOPT_PATH_AS_IS, false);
	set_option(CURLOPT_CRLF, false);
#ifdef _DEBUG
	set_option(CURLOPT_VERBOSE, true);
#else
	set_option(CURLOPT_VERBOSE, false);
#endif
}

own3d::util::curl::~curl()
{
	curl_easy_cleanup(_curl);
}

void own3d::util::curl::clear_headers()
{
	_headers.clear();
}

void own3d::util::curl::clear_header(std::string header)
{
	_headers.erase(header);
}

void own3d::util::curl::set_header(std::string header, std::string value)
{
	_headers.insert_or_assign(header, value);
}

size_t perform_get_kv_size(std::string a, std::string b)
{
	return a.size() + 2 + b.size() + 1;
};

CURLcode own3d::util::curl::perform()
{
	std::vector<char>  buffer;
	struct curl_slist* headers = nullptr;

	if (_headers.size() > 0) {
		// Calculate full buffer size.
		{
			size_t buffer_size = 0;
			for (auto kv : _headers) {
				buffer_size += perform_get_kv_size(kv.first, kv.second);
			}
			buffer.resize(buffer_size * 2);
		}
		// Create HTTP Headers.
		{
			size_t buffer_offset = 0;
			for (auto kv : _headers) {
				size_t size = perform_get_kv_size(kv.first, kv.second);

				snprintf(&buffer.at(buffer_offset), size, "%s: %s", kv.first.c_str(), kv.second.c_str());

				headers = curl_slist_append(headers, &buffer.at(buffer_offset));

				buffer_offset += size;
			}
		}
		set_option<struct curl_slist*>(CURLOPT_HTTPHEADER, headers);
	}

	CURLcode res = curl_easy_perform(_curl);

	if (headers) {
		set_option<struct curl_slist*>(CURLOPT_HTTPHEADER, nullptr);
		curl_slist_free_all(headers);
	}

	return res;
}

void own3d::util::curl::reset()
{
	curl_easy_reset(_curl);
}

CURLcode own3d::util::curl::set_read_callback(curl_io_callback_t cb)
{
	_read_callback = cb;
	if (CURLcode res = curl_easy_setopt(_curl, CURLOPT_READDATA, this); res != CURLE_OK)
		return res;
	return curl_easy_setopt(_curl, CURLOPT_READFUNCTION, &read_helper);
}

CURLcode own3d::util::curl::set_write_callback(curl_io_callback_t cb)
{
	_write_callback = cb;
	if (CURLcode res = curl_easy_setopt(_curl, CURLOPT_WRITEDATA, this); res != CURLE_OK)
		return res;
	return curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, &write_helper);
}

CURLcode own3d::util::curl::set_xferinfo_callback(curl_xferinfo_callback_t cb)
{
	_xferinfo_callback = cb;
	if (CURLcode res = curl_easy_setopt(_curl, CURLOPT_XFERINFODATA, this); res != CURLE_OK)
		return res;
	return curl_easy_setopt(_curl, CURLOPT_XFERINFOFUNCTION, &xferinfo_callback);
}

CURLcode own3d::util::curl::set_debug_callback(curl_debug_callback_t cb)
{
	_debug_callback = cb;
	if (CURLcode res = curl_easy_setopt(_curl, CURLOPT_DEBUGDATA, this); res != CURLE_OK)
		return res;
	return curl_easy_setopt(_curl, CURLOPT_DEBUGFUNCTION, &debug_helper);
}
