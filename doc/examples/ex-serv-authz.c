#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <gnutls/gnutls.h>

#define KEYFILE "key.pem"
#define CERTFILE "cert.pem"
#define CAFILE "ca.pem"
#define CRLFILE "crl.pem"

/* This is a sample TLS 1.0 echo server, using X.509 authentication.
 */


#define SA struct sockaddr
#define SOCKET_ERR(err,s) if(err==-1) {perror(s);return(1);}
#define MAX_BUF 1024
#define PORT 5556		/* listen to 5556 port */
#define DH_BITS 1024

/* These are global */
gnutls_certificate_credentials_t x509_cred;

gnutls_session_t
initialize_tls_session (void)
{
  gnutls_session_t session;

  gnutls_init (&session, GNUTLS_SERVER);

  /* avoid calling all the priority functions, since the defaults
   * are adequate.
   */
  gnutls_set_default_priority (session);

  gnutls_credentials_set (session, GNUTLS_CRD_CERTIFICATE, x509_cred);

  /* request client certificate if any.
   */
  gnutls_certificate_server_set_request (session, GNUTLS_CERT_REQUEST);

  gnutls_dh_set_prime_bits (session, DH_BITS);

  return session;
}

static gnutls_dh_params_t dh_params;

static int
generate_dh_params (void)
{

  /* Generate Diffie Hellman parameters - for use with DHE
   * kx algorithms. These should be discarded and regenerated
   * once a day, once a week or once a month. Depending on the
   * security requirements.
   */
  gnutls_dh_params_init (&dh_params);
  gnutls_dh_params_generate2 (dh_params, DH_BITS);

  return 0;
}

int server_authorized_p = 0;

int
authz_recv_callback (gnutls_session_t session,
		     const int *authz_formats,
		     gnutls_datum_t *infos,
		     const int *hashtypes,
		     gnutls_datum_t *hash)
{
  size_t i, j;

  /* This function receives authorization data. */

  for (i = 0; authz_formats[i]; i++)
    {
      printf ("- Received authorization data, format %02x of %d bytes\n",
	      authz_formats[i], infos[i].size);

      printf ("  data: ");
      for (j = 0; j < infos[i].size; j++)
	printf ("%02x", infos[i].data[j]);
      printf ("\n");

      if (hash[i].size > 0)
	{
	  printf (" hash: ");
	  for (j = 0; j < hash[i].size; j++)
	    printf ("%02x", hash[i].data[j]);
	  printf (" type %02x\n", hashtypes[i]);
	}
    }

  /* You would typically actually _validate_ the data here... if you
     need access to authentication details, store the authorization
     data and do the validation inside main(). */

  server_authorized_p = 1;

  return 0;
}

int
authz_send_callback (gnutls_session_t session,
		     const int *client_formats,
		     const int *server_formats)
{
  const char *str = "saml assertion";
  /* Send the authorization data here. client_formats and
     server_formats contains a list of negotiated authorization
     formats.  */
  return gnutls_authz_send_saml_assertion (session, str, sizeof (str));
}

int
main (void)
{
  int err, listen_sd, i;
  int sd, ret;
  struct sockaddr_in sa_serv;
  struct sockaddr_in sa_cli;
  int client_len;
  char topbuf[512];
  gnutls_session_t session;
  char buffer[MAX_BUF + 1];
  int optval = 1;
  const int authz_server_formats[] = {
    GNUTLS_AUTHZ_SAML_ASSERTION,
  };
  const int authz_client_formats[] = {
    GNUTLS_AUTHZ_X509_ATTR_CERT,
    GNUTLS_AUTHZ_SAML_ASSERTION,
    GNUTLS_AUTHZ_X509_ATTR_CERT_URL,
    GNUTLS_AUTHZ_SAML_ASSERTION_URL
  };

  /* this must be called once in the program
   */
  gnutls_global_init ();

  gnutls_certificate_allocate_credentials (&x509_cred);
  gnutls_certificate_set_x509_trust_file (x509_cred, CAFILE,
					  GNUTLS_X509_FMT_PEM);

  gnutls_certificate_set_x509_crl_file (x509_cred, CRLFILE,
					GNUTLS_X509_FMT_PEM);

  gnutls_certificate_set_x509_key_file (x509_cred, CERTFILE, KEYFILE,
					GNUTLS_X509_FMT_PEM);

  generate_dh_params ();

  gnutls_certificate_set_dh_params (x509_cred, dh_params);

  /* Socket operations
   */
  listen_sd = socket (AF_INET, SOCK_STREAM, 0);
  SOCKET_ERR (listen_sd, "socket");

  memset (&sa_serv, '\0', sizeof (sa_serv));
  sa_serv.sin_family = AF_INET;
  sa_serv.sin_addr.s_addr = INADDR_ANY;
  sa_serv.sin_port = htons (PORT);	/* Server Port number */

  setsockopt (listen_sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof (int));

  err = bind (listen_sd, (SA *) & sa_serv, sizeof (sa_serv));
  SOCKET_ERR (err, "bind");
  err = listen (listen_sd, 1024);
  SOCKET_ERR (err, "listen");

  printf ("Server ready. Listening to port '%d'.\n\n", PORT);

  client_len = sizeof (sa_cli);
  for (;;)
    {
      session = initialize_tls_session ();

      sd = accept (listen_sd, (SA *) & sa_cli, &client_len);

      printf ("- connection from %s, port %d\n",
	      inet_ntop (AF_INET, &sa_cli.sin_addr, topbuf,
			 sizeof (topbuf)), ntohs (sa_cli.sin_port));

      gnutls_transport_set_ptr (session, (gnutls_transport_ptr_t) sd);

      gnutls_authz_enable (session, authz_client_formats, authz_server_formats,
			   authz_recv_callback, authz_send_callback);

      ret = gnutls_handshake (session);
      if (ret < 0)
	{
	  close (sd);
	  gnutls_deinit (session);
	  fprintf (stderr, "*** Handshake has failed (%s)\n\n",
		   gnutls_strerror (ret));
	  continue;
	}

      if (!server_authorized_p)
	{
	  fprintf (stderr, "*** Not authorized, giving up...\n");
	  ret = gnutls_alert_send (session, GNUTLS_AL_FATAL,
				   GNUTLS_A_ACCESS_DENIED);
	  if (ret < 0)
	    continue;
	}

      printf ("- Handshake was completed\n");

      /* see the Getting peer's information example */
      /* print_info(session); */

      i = 0;
      for (;;)
	{
	  memset (buffer, 0, MAX_BUF + 1);
	  ret = gnutls_record_recv (session, buffer, MAX_BUF);

	  if (ret == 0)
	    {
	      printf ("\n- Peer has closed the GNUTLS connection\n");
	      break;
	    }
	  else if (ret < 0)
	    {
	      fprintf (stderr, "\n*** Received corrupted "
		       "data(%d). Closing the connection.\n\n", ret);
	      break;
	    }
	  else if (ret > 0)
	    {
	      /* echo data back to the client
	       */
	      gnutls_record_send (session, buffer, strlen (buffer));
	    }
	}
      printf ("\n");
      /* do not wait for the peer to close the connection.
       */
      gnutls_bye (session, GNUTLS_SHUT_WR);

      close (sd);
      gnutls_deinit (session);

    }
  close (listen_sd);

  gnutls_certificate_free_credentials (x509_cred);

  gnutls_global_deinit ();

  return 0;

}