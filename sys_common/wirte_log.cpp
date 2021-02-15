
#include <cstdarg>
#include <cstdio>
#include <memory>
#include <ctime>
#include <cstdio>

// suitable for ascii. not for unicode
void WriteLog(char *file_name_tag, const char *fmt, ...)
{
	va_list ap;

	const int cst_buf_len = 1024;
	char szContent[cst_buf_len] = {0};
	

	va_start(ap, fmt);
	vsprintf_s(szContent, cst_buf_len, fmt, ap);
	va_end(ap);

	time_t rawtime;
	struct tm * timeinfo;
	time( &rawtime );
	timeinfo = localtime( &rawtime );

	char szFileName[512] = {0};
	sprintf_s( szFileName, sizeof(szFileName), "%s_%4d-%02d-%02d.log", (file_name_tag ? file_name_tag : ""), 1900+timeinfo->tm_year, 1+timeinfo->tm_mon, timeinfo->tm_mday );
	FILE *fp = fopen( szFileName, "a+" );
	if( !fp ) 
		return;
	//char *p_buf = new char[cst_buf_len+256]; 
	//memset(p_buf, 0, cst_buf_len+256);
	
	fprintf( fp, "[%4d-%02d-%02d %02d:%02d:%02d] %s \r\n", \
		1900+timeinfo->tm_year,1+timeinfo->tm_mon,timeinfo->tm_mday,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,
		szContent ); 
	fclose(fp); 
	 
	//delete [] p_buf;
	//emit PrintLogSignal(p_buf);
}
