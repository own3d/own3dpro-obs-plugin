#include "utility.hpp"

QString own3d::util::hex_to_string(QString text)
{
	/* static thread_local */ std::vector<char> buffer;
	buffer.resize(text.length() / 2);

	for (size_t n = 0; n < buffer.size(); n++) {
		QStringRef part{&text, static_cast<int>(n * 2), 2};
		bool       status = false;
		uint       dec    = part.toUInt(&status, 16);
		if (status) {
			buffer[n] = static_cast<char>(dec);
		}
	}

	return QString::fromUtf8(buffer.data(), static_cast<int>(buffer.size()));
}
