

#include "includes.h"

#include "common.h"


#ifdef CONFIG_DEBUG_FILE
static FILE *out_file = NULL;
#endif /* CONFIG_DEBUG_FILE */

int wpa_debug_level = MSG_INFO;

int wpa_debug_show_keys = 0;
int wpa_debug_timestamp = 0;


#ifdef CONFIG_ANDROID_LOG

#include <cutils/xlog.h>

void android_printf(int level, char *format, ...)
{
	va_list ap;
	char *buf;
	const int buflen=2048;
	int cnt=0;
	
	buf = os_malloc(buflen);
	if(buf == NULL)
	{
		xlog_printf(ANDROID_LOG_ERROR, "wpa_supplicant", "android_printf : Failed to allocate massage buffer");
		return;
	}
	va_start(ap, format);
	cnt = vsnprintf(buf, buflen-sizeof(char), format, ap);
	va_end(ap);
	if(cnt < 0 )
	{
		xlog_printf(ANDROID_LOG_ERROR, "wpa_supplicant", "android_printf : Log information is too long");
		return;
	}
	if( level == MSG_MSGDUMP)  xlog_printf(ANDROID_LOG_VERBOSE, "wpa_supplicant", "%s", buf);
	else if( level ==  MSG_DEBUG)	xlog_printf(ANDROID_LOG_DEBUG, "wpa_supplicant", "%s", buf);
	else if( level ==  MSG_INFO)	xlog_printf(ANDROID_LOG_INFO, "wpa_supplicant", "%s", buf);
	else if( level ==  MSG_WARNING)	xlog_printf(ANDROID_LOG_WARN, "wpa_supplicant", "%s", buf);
	else xlog_printf(ANDROID_LOG_ERROR, "wpa_supplicant", "%s", buf);
	os_free(buf);
}

#define LOG_FUNC android_printf
#define UINT_32  u32


void wpa_hexdump(int level, const char *title, const u8 *buf, size_t len)
{
	char *strTmp = NULL;
	u8 *pucStartAddr = buf;
	u32 u4Length = len; 

	LOG_FUNC(level, "%s\n", title);
	LOG_FUNC(level, "DUMP8 ADDRESS: %08lx, Length: %ld\n", (UINT_32)pucStartAddr, u4Length);

	while (u4Length > 0) {
		if (u4Length >= 16) {
		LOG_FUNC(level,"(%08lx) %02x %02x %02x %02x  %02x %02x %02x %02x - %02x %02x %02x %02x  %02x %02x %02x %02x\n",
			(UINT_32)pucStartAddr,
			pucStartAddr[ 0], pucStartAddr[ 1], pucStartAddr[ 2], pucStartAddr[ 3],
			pucStartAddr[ 4], pucStartAddr[ 5], pucStartAddr[ 6], pucStartAddr[ 7],
			pucStartAddr[ 8], pucStartAddr[ 9], pucStartAddr[10], pucStartAddr[11],
			pucStartAddr[12], pucStartAddr[13], pucStartAddr[14], pucStartAddr[15]);
		u4Length -= 16;
		pucStartAddr += 16;
		}
		else {
		switch (u4Length) {
		case 1:
			LOG_FUNC(level, "(%08lx) %02x\n",
			(UINT_32)pucStartAddr,
			pucStartAddr[ 0]);
			break;
		case 2:
			LOG_FUNC(level, "(%08lx) %02x %02x\n",
			(UINT_32)pucStartAddr,
			pucStartAddr[ 0], pucStartAddr[ 1]);
			break;
		case 3:
			LOG_FUNC(level, "(%08lx) %02x %02x %02x\n",
			(UINT_32)pucStartAddr,
			pucStartAddr[ 0], pucStartAddr[ 1], pucStartAddr[ 2]);
			break;
		case 4:
			LOG_FUNC(level, "(%08lx) %02x %02x %02x %02x\n",
			(UINT_32)pucStartAddr,
			pucStartAddr[ 0], pucStartAddr[ 1], pucStartAddr[ 2], pucStartAddr[ 3]);
			break;
		case 5:
			LOG_FUNC(level, "(%08lx) %02x %02x %02x %02x  %02x\n",
			(UINT_32)pucStartAddr,
			pucStartAddr[ 0], pucStartAddr[ 1], pucStartAddr[ 2], pucStartAddr[ 3],
			pucStartAddr[ 4]);
			break;
		case 6:
			LOG_FUNC(level, "(%08lx) %02x %02x %02x %02x  %02x %02x\n",
			(UINT_32)pucStartAddr,
			pucStartAddr[ 0], pucStartAddr[ 1], pucStartAddr[ 2], pucStartAddr[ 3],
			pucStartAddr[ 4], pucStartAddr[ 5]);
			break;
		case 7:
			LOG_FUNC(level, "(%08lx) %02x %02x %02x %02x  %02x %02x %02x\n",
			(UINT_32)pucStartAddr,
			pucStartAddr[ 0], pucStartAddr[ 1], pucStartAddr[ 2], pucStartAddr[ 3],
			pucStartAddr[ 4], pucStartAddr[ 5], pucStartAddr[ 6]);
			break;
		case 8:
			LOG_FUNC(level, "(%08lx) %02x %02x %02x %02x  %02x %02x %02x %02x\n",
			(UINT_32)pucStartAddr,
			pucStartAddr[ 0], pucStartAddr[ 1], pucStartAddr[ 2], pucStartAddr[ 3],
			pucStartAddr[ 4], pucStartAddr[ 5], pucStartAddr[ 6], pucStartAddr[ 7]);
			break;
		case 9:
			LOG_FUNC(level, "(%08lx) %02x %02x %02x %02x  %02x %02x %02x %02x - %02x\n",
			(UINT_32)pucStartAddr,
			pucStartAddr[ 0], pucStartAddr[ 1], pucStartAddr[ 2], pucStartAddr[ 3],
			pucStartAddr[ 4], pucStartAddr[ 5], pucStartAddr[ 6], pucStartAddr[ 7],
			pucStartAddr[ 8]);
			break;
		case 10:
			LOG_FUNC(level, "(%08lx) %02x %02x %02x %02x  %02x %02x %02x %02x - %02x %02x\n",
			(UINT_32)pucStartAddr,
			pucStartAddr[ 0], pucStartAddr[ 1], pucStartAddr[ 2], pucStartAddr[ 3],
			pucStartAddr[ 4], pucStartAddr[ 5], pucStartAddr[ 6], pucStartAddr[ 7],
			pucStartAddr[ 8], pucStartAddr[ 9]);
			break;
		case 11:
			LOG_FUNC(level, "(%08lx) %02x %02x %02x %02x  %02x %02x %02x %02x - %02x %02x %02x\n",
			(UINT_32)pucStartAddr,
			pucStartAddr[ 0], pucStartAddr[ 1], pucStartAddr[ 2], pucStartAddr[ 3],
			pucStartAddr[ 4], pucStartAddr[ 5], pucStartAddr[ 6], pucStartAddr[ 7],
			pucStartAddr[ 8], pucStartAddr[ 9], pucStartAddr[10]);
			break;
		case 12:
			LOG_FUNC(level, "(%08lx) %02x %02x %02x %02x  %02x %02x %02x %02x - %02x %02x %02x %02x\n",
			(UINT_32)pucStartAddr,
			pucStartAddr[ 0], pucStartAddr[ 1], pucStartAddr[ 2], pucStartAddr[ 3],
			pucStartAddr[ 4], pucStartAddr[ 5], pucStartAddr[ 6], pucStartAddr[ 7],
			pucStartAddr[ 8], pucStartAddr[ 9], pucStartAddr[10], pucStartAddr[11]);
			break;
		case 13:
			LOG_FUNC(level, "(%08lx) %02x %02x %02x %02x  %02x %02x %02x %02x - %02x %02x %02x %02x  %02x\n",
			(UINT_32)pucStartAddr,
			pucStartAddr[ 0], pucStartAddr[ 1], pucStartAddr[ 2], pucStartAddr[ 3],
			pucStartAddr[ 4], pucStartAddr[ 5], pucStartAddr[ 6], pucStartAddr[ 7],
			pucStartAddr[ 8], pucStartAddr[ 9], pucStartAddr[10], pucStartAddr[11],
			pucStartAddr[12]);
			break;
		case 14:
			LOG_FUNC(level, "(%08lx) %02x %02x %02x %02x  %02x %02x %02x %02x - %02x %02x %02x %02x  %02x %02x\n",
			(UINT_32)pucStartAddr,
			pucStartAddr[ 0], pucStartAddr[ 1], pucStartAddr[ 2], pucStartAddr[ 3],
			pucStartAddr[ 4], pucStartAddr[ 5], pucStartAddr[ 6], pucStartAddr[ 7],
			pucStartAddr[ 8], pucStartAddr[ 9], pucStartAddr[10], pucStartAddr[11],
			pucStartAddr[12], pucStartAddr[13]);
			break;
		case 15:
			LOG_FUNC(level, "(%08lx) %02x %02x %02x %02x  %02x %02x %02x %02x - %02x %02x %02x %02x  %02x %02x %02x\n",
			(UINT_32)pucStartAddr,
			pucStartAddr[ 0], pucStartAddr[ 1], pucStartAddr[ 2], pucStartAddr[ 3],
			pucStartAddr[ 4], pucStartAddr[ 5], pucStartAddr[ 6], pucStartAddr[ 7],
			pucStartAddr[ 8], pucStartAddr[ 9], pucStartAddr[10], pucStartAddr[11],
			pucStartAddr[12], pucStartAddr[13], pucStartAddr[14]);
			break;
		default:
			break;
		}
		u4Length = 0;
		}
	}

    	LOG_FUNC(level, "\n");	

}

#else /* CONFIG_ANDROID_LOG */

#ifndef CONFIG_NO_STDOUT_DEBUG

void wpa_debug_print_timestamp(void)
{
	struct os_time tv;

	if (!wpa_debug_timestamp)
		return;

	os_get_time(&tv);
#ifdef CONFIG_DEBUG_FILE
	if (out_file) {
		fprintf(out_file, "%ld.%06u: ", (long) tv.sec,
			(unsigned int) tv.usec);
	} else
#endif /* CONFIG_DEBUG_FILE */
	printf("%ld.%06u: ", (long) tv.sec, (unsigned int) tv.usec);
}


void wpa_printf(int level, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	if (level >= wpa_debug_level) {
		wpa_debug_print_timestamp();
#ifdef CONFIG_DEBUG_FILE
		if (out_file) {
			vfprintf(out_file, fmt, ap);
			fprintf(out_file, "\n");
		} else {
#endif /* CONFIG_DEBUG_FILE */
		vprintf(fmt, ap);
		printf("\n");
#ifdef CONFIG_DEBUG_FILE
		}
#endif /* CONFIG_DEBUG_FILE */
	}
	va_end(ap);
}


static void _wpa_hexdump(int level, const char *title, const u8 *buf,
			 size_t len, int show)
{
	size_t i;
	if (level < wpa_debug_level)
		return;
	wpa_debug_print_timestamp();
#ifdef CONFIG_DEBUG_FILE
	if (out_file) {
		fprintf(out_file, "%s - hexdump(len=%lu):",
			title, (unsigned long) len);
		if (buf == NULL) {
			fprintf(out_file, " [NULL]");
		} else if (show) {
			for (i = 0; i < len; i++)
				fprintf(out_file, " %02x", buf[i]);
		} else {
			fprintf(out_file, " [REMOVED]");
		}
		fprintf(out_file, "\n");
	} else {
#endif /* CONFIG_DEBUG_FILE */
	printf("%s - hexdump(len=%lu):", title, (unsigned long) len);
	if (buf == NULL) {
		printf(" [NULL]");
	} else if (show) {
		for (i = 0; i < len; i++)
			printf(" %02x", buf[i]);
	} else {
		printf(" [REMOVED]");
	}
	printf("\n");
#ifdef CONFIG_DEBUG_FILE
	}
#endif /* CONFIG_DEBUG_FILE */
}

void wpa_hexdump(int level, const char *title, const u8 *buf, size_t len)
{
	_wpa_hexdump(level, title, buf, len, 1);
}


void wpa_hexdump_key(int level, const char *title, const u8 *buf, size_t len)
{
	_wpa_hexdump(level, title, buf, len, wpa_debug_show_keys);
}


static void _wpa_hexdump_ascii(int level, const char *title, const u8 *buf,
			       size_t len, int show)
{
	size_t i, llen;
	const u8 *pos = buf;
	const size_t line_len = 16;

	if (level < wpa_debug_level)
		return;
	wpa_debug_print_timestamp();
#ifdef CONFIG_DEBUG_FILE
	if (out_file) {
		if (!show) {
			fprintf(out_file,
				"%s - hexdump_ascii(len=%lu): [REMOVED]\n",
				title, (unsigned long) len);
			return;
		}
		if (buf == NULL) {
			fprintf(out_file,
				"%s - hexdump_ascii(len=%lu): [NULL]\n",
				title, (unsigned long) len);
			return;
		}
		fprintf(out_file, "%s - hexdump_ascii(len=%lu):\n",
			title, (unsigned long) len);
		while (len) {
			llen = len > line_len ? line_len : len;
			fprintf(out_file, "    ");
			for (i = 0; i < llen; i++)
				fprintf(out_file, " %02x", pos[i]);
			for (i = llen; i < line_len; i++)
				fprintf(out_file, "   ");
			fprintf(out_file, "   ");
			for (i = 0; i < llen; i++) {
				if (isprint(pos[i]))
					fprintf(out_file, "%c", pos[i]);
				else
					fprintf(out_file, "_");
			}
			for (i = llen; i < line_len; i++)
				fprintf(out_file, " ");
			fprintf(out_file, "\n");
			pos += llen;
			len -= llen;
		}
	} else {
#endif /* CONFIG_DEBUG_FILE */
	if (!show) {
		printf("%s - hexdump_ascii(len=%lu): [REMOVED]\n",
		       title, (unsigned long) len);
		return;
	}
	if (buf == NULL) {
		printf("%s - hexdump_ascii(len=%lu): [NULL]\n",
		       title, (unsigned long) len);
		return;
	}
	printf("%s - hexdump_ascii(len=%lu):\n", title, (unsigned long) len);
	while (len) {
		llen = len > line_len ? line_len : len;
		printf("    ");
		for (i = 0; i < llen; i++)
			printf(" %02x", pos[i]);
		for (i = llen; i < line_len; i++)
			printf("   ");
		printf("   ");
		for (i = 0; i < llen; i++) {
			if (isprint(pos[i]))
				printf("%c", pos[i]);
			else
				printf("_");
		}
		for (i = llen; i < line_len; i++)
			printf(" ");
		printf("\n");
		pos += llen;
		len -= llen;
	}
#ifdef CONFIG_DEBUG_FILE
	}
#endif /* CONFIG_DEBUG_FILE */
}


void wpa_hexdump_ascii(int level, const char *title, const u8 *buf, size_t len)
{
	_wpa_hexdump_ascii(level, title, buf, len, 1);
}


void wpa_hexdump_ascii_key(int level, const char *title, const u8 *buf,
			   size_t len)
{
	_wpa_hexdump_ascii(level, title, buf, len, wpa_debug_show_keys);
}


int wpa_debug_open_file(const char *path)
{
#ifdef CONFIG_DEBUG_FILE
	if (!path)
		return 0;
	out_file = fopen(path, "a");
	if (out_file == NULL) {
		wpa_printf(MSG_ERROR, "wpa_debug_open_file: Failed to open "
			   "output file, using standard output");
		return -1;
	}
#ifndef _WIN32
	setvbuf(out_file, NULL, _IOLBF, 0);
#endif /* _WIN32 */
#endif /* CONFIG_DEBUG_FILE */
	return 0;
}


void wpa_debug_close_file(void)
{
#ifdef CONFIG_DEBUG_FILE
	if (!out_file)
		return;
	fclose(out_file);
	out_file = NULL;
#endif /* CONFIG_DEBUG_FILE */
}

#endif /* CONFIG_NO_STDOUT_DEBUG */

#endif /* CONFIG_ANDROID_LOG */

#ifndef CONFIG_NO_WPA_MSG
static wpa_msg_cb_func wpa_msg_cb = NULL;

void wpa_msg_register_cb(wpa_msg_cb_func func)
{
	wpa_msg_cb = func;
}


void wpa_msg(void *ctx, int level, const char *fmt, ...)
{
	va_list ap;
	char *buf;
	const int buflen = 2048;
	int len;

	buf = os_malloc(buflen);
	if (buf == NULL) {
		wpa_printf(MSG_ERROR, "wpa_msg: Failed to allocate message "
			   "buffer");
		return;
	}
	va_start(ap, fmt);
	len = vsnprintf(buf, buflen, fmt, ap);
	va_end(ap);
	wpa_printf(level, "%s", buf);
	if (wpa_msg_cb)
		wpa_msg_cb(ctx, level, buf, len);
	os_free(buf);
}


void wpa_msg_ctrl(void *ctx, int level, const char *fmt, ...)
{
	va_list ap;
	char *buf;
	const int buflen = 2048;
	int len;

	if (!wpa_msg_cb)
		return;

	buf = os_malloc(buflen);
	if (buf == NULL) {
		wpa_printf(MSG_ERROR, "wpa_msg_ctrl: Failed to allocate "
			   "message buffer");
		return;
	}
	va_start(ap, fmt);
	len = vsnprintf(buf, buflen, fmt, ap);
	va_end(ap);
	wpa_msg_cb(ctx, level, buf, len);
	os_free(buf);
}
#endif /* CONFIG_NO_WPA_MSG */


#ifndef CONFIG_NO_HOSTAPD_LOGGER
static hostapd_logger_cb_func hostapd_logger_cb = NULL;

void hostapd_logger_register_cb(hostapd_logger_cb_func func)
{
	hostapd_logger_cb = func;
}


void hostapd_logger(void *ctx, const u8 *addr, unsigned int module, int level,
		    const char *fmt, ...)
{
	va_list ap;
	char *buf;
	const int buflen = 2048;
	int len;

	buf = os_malloc(buflen);
	if (buf == NULL) {
		wpa_printf(MSG_ERROR, "hostapd_logger: Failed to allocate "
			   "message buffer");
		return;
	}
	va_start(ap, fmt);
	len = vsnprintf(buf, buflen, fmt, ap);
	va_end(ap);
	if (hostapd_logger_cb)
		hostapd_logger_cb(ctx, addr, module, level, buf, len);
	else
		wpa_printf(MSG_DEBUG, "hostapd_logger: %s", buf);
	os_free(buf);
}
#endif /* CONFIG_NO_HOSTAPD_LOGGER */

#ifdef CONFIG_WAPI_SUPPORT
void print_buf(const char* title, const void* buf, int len)
{
    
}

void buffer_hexdump(int level, const char *title, const u8 *buf,
			 size_t len)
{	
	if(!buf)
	{
		wpa_printf(MSG_ERROR, "buffer_hexdump receive NULL pointer!!\n");
		return;
	}
		
	if (level >= MSG_MSGDUMP)
	{
		print_buf(title, buf, len);
	}
}
#endif