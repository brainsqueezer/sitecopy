/*****************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * $Id: ftpget.c,v 1.7 2008/02/27 09:06:15 bagder Exp $
 */



/*
curl-config --libs
gcc curlftpget.c -lcurl -lgssapi_krb5



CURLOPT_PROXY

Set HTTP proxy to use. The parameter should be a char * to a zero terminated string holding the host name or dotted IP address. To specify port number in this string, append :[port] to the end of the host name. The proxy string may be prefixed with [protocol]:// since any such prefix will be ignored. The proxy's port number may optionally be specified with the separate option CURLOPT_PROXYPORT.

When you tell the library to use an HTTP proxy, libcurl will transparently convert operations to HTTP even if you specify an FTP URL etc. This may have an impact on what other features of the library you can use, such as CURLOPT_QUOTE and similar FTP specifics that don't work unless you tunnel through the HTTP proxy. Such tunneling is activated with CURLOPT_HTTPPROXYTUNNEL.

libcurl respects the environment variables http_proxy, ftp_proxy, all_proxy etc, if any of those is set. The CURLOPT_PROXY option does however override any possibly set environment variables.

Starting with 7.14.1, the proxy host string can be specified the exact same way as the proxy environment variables, include protocol prefix (http://) and embedded user + password.

CURLOPT_PROXYPORT

Pass a long with this option to set the proxy port to connect to unless it is specified in the proxy string CURLOPT_PROXY.

CURLOPT_PROXYTYPE

Pass a long with this option to set type of the proxy. Available options for this are CURLPROXY_HTTP, CURLPROXY_SOCKS4 (added in 7.15.2) CURLPROXY_SOCKS5. The HTTP type is default. (Added in 7.10)

CURLOPT_HTTPPROXYTUNNEL

Set the parameter to non-zero to get the library to tunnel all operations through a given HTTP proxy. There is a big difference between using a proxy and to tunnel through it. If you don't know what this means, you probably don't want this tunneling option.



*/






#include <stdio.h>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

/*
 * This is an example showing how to get a single file from an FTP server.
 * It delays the actual destination file creation until the first write
 * callback so that it won't create an empty file in case the remote file
 * doesn't exist or something else fails.
 */

struct FtpFile {
  const char *filename;
  FILE *stream;
};

static size_t my_fwrite(void *buffer, size_t size, size_t nmemb, void *stream)
{
  struct FtpFile *out=(struct FtpFile *)stream;
  if(out && !out->stream) {
    /* open file for writing */
    out->stream=fopen(out->filename, "wb");
    if(!out->stream)
      return -1; /* failure, can't open file to write */
  }
  return fwrite(buffer, size, nmemb, out->stream);
}


int main(void)
{
  CURL *curl;
  CURLcode res;
  struct FtpFile ftpfile={
    "curl.tar.gz", /* name to store the file as if succesful */
    NULL
  };

  curl_global_init(CURL_GLOBAL_DEFAULT);

  curl = curl_easy_init();
  if(curl) {
    /*
     * Get curl 7.9.2 from sunet.se's FTP site. curl 7.9.2 is most likely not
     * present there by the time you read this, so you'd better replace the
     * URL with one that works!
     */
    curl_easy_setopt(curl, CURLOPT_URL,
                     "ftp://ftp.sunet.se/pub/www/utilities/curl/curl-7.9.2.tar.gz");
    /* Define our callback to get called when there's data to be written */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_fwrite);
    /* Set a pointer to our struct to pass to the callback */
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftpfile);

    /* Switch on full protocol/debug output */
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

    res = curl_easy_perform(curl);

    /* always cleanup */
    curl_easy_cleanup(curl);

    if(CURLE_OK != res) {
      /* we failed */
      fprintf(stderr, "curl told us %d\n", res);
    }
  }

  if(ftpfile.stream)
    fclose(ftpfile.stream); /* close the local file */

  curl_global_cleanup();

  return 0;
}
