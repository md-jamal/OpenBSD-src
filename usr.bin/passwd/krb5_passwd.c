/*
 * Copyright (c) 1997-2000 Kungliga Tekniska H�gskolan
 * (Royal Institute of Technology, Stockholm, Sweden).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <dlfcn.h>
#include <util.h>
#include <err.h>
#include <openssl/ui.h>
#include <kerberosV/krb5.h>

/* RCSID("$KTH: kpasswd.c,v 1.23 2000/12/31 07:48:34 assar Exp $"); */

int krb5_passwd(int, char **);

int
krb5_passwd(int argc, char **argv)
{
	krb5_data result_code_string, result_string;
	krb5_get_init_creds_opt *opt;
	krb5_principal principal;
	krb5_context context;
	krb5_error_code ret;
	char pwbuf[BUFSIZ];
	krb5_creds cred;
	int result_code;
	uid_t uid;

	uid = getuid();
	if (setresuid(uid, uid, uid)) {
		errx(1, "can't drop privileges\n");
	}

	ret = krb5_init_context(&context);
	if (ret)
		errx(1, "krb5_init_context failed: %d", ret);

	ret = krb5_get_init_creds_opt_alloc (context, &opt);
	if (ret)
		errx(1, "krb5_get_init_creds_opt_alloc failed: %d", ret);

	krb5_get_init_creds_opt_set_tkt_life (opt, 300);
	krb5_get_init_creds_opt_set_forwardable (opt, FALSE);
	krb5_get_init_creds_opt_set_proxiable (opt, FALSE);

	if (argv[0]) {
		ret = krb5_parse_name(context, argv[0], &principal);
		if (ret) {
			krb5_get_init_creds_opt_free(context, opt);
			krb5_err(context, 1, ret, "krb5_parse_name");
		}
	} else {
		ret = krb5_get_default_principal (context, &principal);
		if (ret) {
			krb5_get_init_creds_opt_free(context, opt);
			krb5_err (context, 1, ret, "krb5_get_default_principal");
		}
        }

	ret = krb5_get_init_creds_password (context, &cred,
	    principal, NULL, krb5_prompter_posix, NULL, 0,
	    "kadmin/changepw", opt);
	krb5_get_init_creds_opt_free (context, opt);
	switch (ret) {
	case 0:
		break;
	case KRB5_LIBOS_PWDINTR :
		return 1;
	case KRB5KRB_AP_ERR_BAD_INTEGRITY :
	case KRB5KRB_AP_ERR_MODIFIED :
		krb5_errx(context, 1, "Password incorrect");
		break;
	default:
		krb5_err(context, 1, ret, "krb5_get_init_creds");
	}

	krb5_data_zero(&result_code_string);
	krb5_data_zero(&result_string);

	if (UI_UTIL_read_pw_string(pwbuf, sizeof(pwbuf), "New password:",
	    1) != 0)
		return 1;

	ret = krb5_set_password (context, &cred, pwbuf, NULL,
				 &result_code,
				 &result_code_string,
				 &result_string);
	if (ret)
		krb5_err(context, 1, ret, "krb5_set_password");

	printf ("%s%s%.*s\n",
	    krb5_passwd_result_to_string(context, result_code),
	    result_string.length > 0 ? " : " : "",
	    (int)result_string.length,
	    result_string.length > 0 ? (char *)result_string.data : "");

	krb5_data_free(&result_code_string);
	krb5_data_free(&result_string);

	krb5_free_cred_contents(context, &cred);
	krb5_free_context(context);
	return result_code;
}
