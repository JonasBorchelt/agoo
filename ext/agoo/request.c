// Copyright (c) 2018, Peter Ohler, All rights reserved.

#include <stdio.h>

#include "con.h"
#include "error_stream.h"
#include "request.h"

static VALUE	req_class = Qundef;

static VALUE	connect_val = Qundef;
static VALUE	content_length_val = Qundef;
static VALUE	content_type_val = Qundef;
static VALUE	delete_val = Qundef;
static VALUE	empty_val = Qundef;
static VALUE	get_val = Qundef;
static VALUE	head_val = Qundef;
static VALUE	http_val = Qundef;
static VALUE	options_val = Qundef;
static VALUE	path_info_val = Qundef;
static VALUE	post_val = Qundef;
static VALUE	put_val = Qundef;
static VALUE	query_string_val = Qundef;
static VALUE	rack_errors_val = Qundef;
static VALUE	rack_input_val = Qundef;
static VALUE	rack_multiprocess_val = Qundef;
static VALUE	rack_multithread_val = Qundef;
static VALUE	rack_run_once_val = Qundef;
static VALUE	rack_url_scheme_val = Qundef;
static VALUE	rack_version_val = Qundef;
static VALUE	rack_version_val_val = Qundef;
static VALUE	request_method_val = Qundef;
static VALUE	script_name_val = Qundef;
static VALUE	server_name_val = Qundef;
static VALUE	server_port_val = Qundef;
static VALUE	slash_val = Qundef;

static VALUE	stringio_class = Qundef;

static ID	new_id;

static const char	content_type[] = "Content-Type";
static const char	content_length[] = "Content-Length";

static VALUE
req_method(Req r) {
    VALUE	m;

    if (NULL == r) {
	rb_raise(rb_eArgError, "Request is no longer valid.");
    }
    switch (r->method) {
    case CONNECT:	m = connect_val;	break;
    case DELETE:	m = delete_val;		break;
    case GET:		m = get_val;		break;
    case HEAD:		m = head_val;		break;
    case OPTIONS:	m = options_val;	break;
    case POST:		m = post_val;		break;
    case PUT:		m = put_val;		break;
    default:		m = Qnil;		break;
    }
    return m;
}

/* Document-method: request_method
 *
 * call-seq: request_method()
 *
 * Returns the HTTP method of the request.
 */
static VALUE
method(VALUE self) {
    return req_method((Req)DATA_PTR(self));
}

static VALUE
req_script_name(Req r) {
    // The logic is a bit tricky here and for path_info. If the HTTP path is /
    // then the script_name must be empty and the path_info will be /. All
    // other cases are handled with the full path in script_name and path_info
    // empty.
    if (NULL == r) {
	rb_raise(rb_eArgError, "Request is no longer valid.");
    }
    if (0 == r->path.len || (1 == r->path.len && '/' == *r->path.start)) {
	return empty_val;
    }
    return rb_str_new(r->path.start, r->path.len);
}

/* Document-method: script_name
 *
 * call-seq: script_name()
 *
 * Returns the path info which is assumed to be the full path unless the root
 * and then the rack restrictions are followed on what the script name and
 * path info should be.
 */
static VALUE
script_name(VALUE self) {
    return req_script_name((Req)DATA_PTR(self));
}

static VALUE
req_path_info(Req r) {
    if (NULL == r) {
	rb_raise(rb_eArgError, "Request is no longer valid.");
    }
    if (0 == r->path.len || (1 == r->path.len && '/' == *r->path.start)) {
	return slash_val;
    }
    return empty_val;
}

/* Document-method: path_info
 *
 * call-seq: path_info()
 *
 * Returns the script name which is assumed to be either '/' or the empty
 * according to the rack restrictions are followed on what the script name and
 * path info should be.
 */
static VALUE
path_info(VALUE self) {
    return req_path_info((Req)DATA_PTR(self));
}

static VALUE
req_query_string(Req r) {
    if (NULL == r) {
	rb_raise(rb_eArgError, "Request is no longer valid.");
    }
    if (NULL == r->query.start) {
	return empty_val;
    }
    return rb_str_new(r->query.start, r->query.len);
}

/* Document-method: query_string
 *
 * call-seq: query_string()
 *
 * Returns the query string of the request.
 */
static VALUE
query_string(VALUE self) {
    return req_query_string((Req)DATA_PTR(self));
}

static VALUE
req_server_name(Req r) {
    int		len;
    const char	*host;
    const char	*colon;

    if (NULL == r) {
	rb_raise(rb_eArgError, "Request is no longer valid.");
    }
    if (NULL == (host = con_header_value(r->header.start, r->header.len, "Host", &len))) {
	return Qnil;
    }
    for (colon = host + len - 1; host < colon; colon--) {
	if (':' == *colon) {
	    break;
	}
    }
    if (host == colon) {
	return Qnil;
    }
    return rb_str_new(host, colon - host);
}

/* Document-method: server_name
 *
 * call-seq: server_name()
 *
 * Returns the server or host name.
 */
static VALUE
server_name(VALUE self) {
    return req_server_name((Req)DATA_PTR(self));
}

static VALUE
req_server_port(Req r) {
    int		len;
    const char	*host;
    const char	*colon;

    if (NULL == r) {
	rb_raise(rb_eArgError, "Request is no longer valid.");
    }
    if (NULL == (host = con_header_value(r->header.start, r->header.len, "Host", &len))) {
	return Qnil;
    }
    for (colon = host + len - 1; host < colon; colon--) {
	if (':' == *colon) {
	    break;
	}
    }
    if (host == colon) {
	return Qnil;
    }
    return rb_str_new(colon + 1, host + len - colon - 1);
}

/* Document-method: server_port
 *
 * call-seq: server_port()
 *
 * Returns the server or host port as a string.
 */
static VALUE
server_port(VALUE self) {
    return req_server_port((Req)DATA_PTR(self));
}

/* Document-method: rack_version
 *
 * call-seq: rack_version()
 *
 * Returns the rack version the request is compliant with.
 */
static VALUE
rack_version(VALUE self) {
    return rack_version_val_val;
}

static VALUE
req_rack_url_scheme(Req r) {
    // TBD http or https when ssl is supported
    return http_val;
}

/* Document-method: rack_url_scheme
 *
 * call-seq: rack_url_scheme()
 *
 * Returns the URL scheme or either _http_ or _https_ as a string.
 */
static VALUE
rack_url_scheme(VALUE self) {
    return req_rack_url_scheme((Req)DATA_PTR(self));
}

static VALUE
req_rack_input(Req r) {
    if (NULL == r) {
	rb_raise(rb_eArgError, "Request is no longer valid.");
    }
    if (NULL == r->body.start) {
	return Qnil;
    }
    return rb_funcall(stringio_class, new_id, 1, rb_str_new(r->body.start, r->body.len));
}

/* Document-method: rack_input
 *
 * call-seq: rack_input()
 *
 * Returns an input stream for the request body. If no body is present then
 * _nil_ is returned.
 */
static VALUE
rack_input(VALUE self) {
    return req_rack_input((Req)DATA_PTR(self));
}

static VALUE
req_rack_errors(Req r) {
    return error_stream_new(r->server);
}

/* Document-method: rack_errors
 *
 * call-seq: rack_errors()
 *
 * Returns an error stream for the request. This stream is used to write error
 * log entries.
 */
static VALUE
rack_errors(VALUE self) {
    return req_rack_errors((Req)DATA_PTR(self));
}

static VALUE
req_rack_multithread(Req r) {
    if (NULL == r) {
	rb_raise(rb_eArgError, "Request is no longer valid.");
    }
    if (NULL != r->server && 1 < r->server->thread_cnt) {
	return Qtrue;
    }
    return Qfalse;
}

/* Document-method: rack_multithread
 *
 * call-seq: rack_multithread()
 *
 * Returns true is the server is using multiple handler worker threads.
 */
static VALUE
rack_multithread(VALUE self) {
    return req_rack_multithread((Req)DATA_PTR(self));
}

/* Document-method: rack_multiprocess
 *
 * call-seq: rack_multiprocess()
 *
 * Returns false since the server is a single process.
 */
static VALUE
rack_multiprocess(VALUE self) {
    return Qfalse;
}

/* Document-method: rack_run_once
 *
 * call-seq: rack_run_once()
 *
 * Returns false.
 */
static VALUE
rack_run_once(VALUE self) {
    return Qfalse;
}

static void
add_header_value(VALUE hh, const char *key, int klen, const char *val, int vlen) {
    if (sizeof(content_type) - 1 == klen && 0 == strncasecmp(key, content_type, sizeof(content_type) - 1)) {
	rb_hash_aset(hh, content_type_val, rb_str_new(val, vlen));
    } else if (sizeof(content_length) - 1 == klen && 0 == strncasecmp(key, content_length, sizeof(content_length) - 1)) {
	rb_hash_aset(hh, content_length_val, rb_str_new(val, vlen));
    } else {
	char	hkey[1024];
	char	*k = hkey;

	strcpy(hkey, "HTTP_");
	k = hkey + 5;
	if ((int)(sizeof(hkey) - 5) <= klen) {
	    klen = sizeof(hkey) - 6;
	}
	strncpy(k, key, klen);
	hkey[klen + 5] = '\0';
    
	rb_hash_aset(hh, rb_str_new(hkey, klen + 5), rb_str_new(val, vlen));
    }
}

static void
fill_headers(Req r, VALUE hash) {
    char		*h = r->header.start;
    char		*end = h + r->header.len;
    char		*key = h;
    char		*kend = key;
    char		*val = NULL;
    char		*vend;
    
    if (NULL == r) {
	rb_raise(rb_eArgError, "Request is no longer valid.");
    }
    for (; h < end; h++) {
	switch (*h) {
	case ':':
	    kend = h;
	    val = h + 1;
	    break;
	case ' ':
	    if (NULL != val) {
		val++;
	    } else {
		// TBD handle trailing spaces as well
		key++;
	    }
	    break;
	case '\r':
	    if (NULL != val) {
		vend = h;
	    }
	    if ('\n' == *(h + 1)) {
		h++;
	    }
	    add_header_value(hash, key, (int)(kend - key), val, (int)(vend - val));
	    key = h + 1;
	    kend = NULL;
	    val = NULL;
	    vend = NULL;
	    break;
	default:
	    break;
	}
    }
}

/* Document-method: headers
 *
 * call-seq: headers()
 *
 * Returns the header of the request as a Hash.
 */
static VALUE
headers(VALUE self) {
    Req			r = DATA_PTR(self);
    volatile VALUE	h;

    if (NULL == r) {
	rb_raise(rb_eArgError, "Request is no longer valid.");
    }
    h = rb_hash_new();
    fill_headers(r, h);

    return h;
}

/* Document-method: body
 *
 * call-seq: body()
 *
 * Returns the body of the request as a String. If there is no body then _nil_
 * is returned.
 */
static VALUE
body(VALUE self) {
    Req		r = DATA_PTR(self);

    if (NULL == r) {
	rb_raise(rb_eArgError, "Request is no longer valid.");
    }
    if (NULL == r->body.start) {
	return Qnil;
    }
    return rb_str_new(r->body.start, r->body.len);
}

/* Document-class: Agoo::Request
 *
 * A Request is passes to handler that respond to the _on_request_ method. The
 * request is a more efficient encapsulation of the rack environment.
 */
VALUE
request_env(Req req) {
    volatile VALUE	env = rb_hash_new();
    
    // As described by
    // http://www.rubydoc.info/github/rack/rack/master/file/SPEC and
    // https://github.com/rack/rack/blob/master/SPEC.

    rb_hash_aset(env, request_method_val, req_method(req));
    rb_hash_aset(env, script_name_val, req_script_name(req));
    rb_hash_aset(env, path_info_val, req_path_info(req));
    rb_hash_aset(env, query_string_val, req_query_string(req));
    rb_hash_aset(env, server_name_val, req_server_name(req));
    rb_hash_aset(env, server_port_val, req_server_port(req));
    fill_headers(req, env);
    rb_hash_aset(env, rack_version_val, rack_version_val_val);
    rb_hash_aset(env, rack_url_scheme_val, req_rack_url_scheme(req));
    rb_hash_aset(env, rack_input_val, req_rack_input(req));
    rb_hash_aset(env, rack_errors_val, req_rack_errors(req));
    rb_hash_aset(env, rack_multithread_val, req_rack_multithread(req));
    rb_hash_aset(env, rack_multiprocess_val, Qfalse);
    rb_hash_aset(env, rack_run_once_val, Qfalse);

    return env;
}

/* Document-method: to_h
 *
 * call-seq: to_h()
 *
 * Returns a Hash representation of the request which is the same as a rack
 * environment Hash.
 */
static VALUE
to_h(VALUE self) {
    Req		r = DATA_PTR(self);

    if (NULL == r) {
	rb_raise(rb_eArgError, "Request is no longer valid.");
    }
    return request_env(r);
}

/* Document-method: to_s
 *
 * call-seq: to_s()
 *
 * Returns a string representation of the request.
 */
static VALUE
to_s(VALUE self) {
    volatile VALUE	h = to_h(self);

    return rb_funcall(h, rb_intern("to_s"), 0);
}

VALUE
request_wrap(Req req) {
    return Data_Wrap_Struct(req_class, NULL, NULL, req);
}

/* Document-class: Agoo::Request
 *
 * A representation of an HTTP request that is used with a handler that
 * responds to the _on_request_ method. The request is a more efficient
 * encapsulation of the rack environment.
 */
void
request_init(VALUE mod) {
    req_class = rb_define_class_under(mod, "Request", rb_cObject);

    rb_define_method(req_class, "to_s", to_s, 0);
    rb_define_method(req_class, "to_h", to_h, 0);
    rb_define_method(req_class, "environment", to_h, 0);
    rb_define_method(req_class, "env", to_h, 0);
    rb_define_method(req_class, "request_method", method, 0);
    rb_define_method(req_class, "script_name", script_name, 0);
    rb_define_method(req_class, "path_info", path_info, 0);
    rb_define_method(req_class, "query_string", query_string, 0);
    rb_define_method(req_class, "server_name", server_name, 0);
    rb_define_method(req_class, "server_port", server_port, 0);
    rb_define_method(req_class, "rack_version", rack_version, 0);
    rb_define_method(req_class, "rack_url_scheme", rack_url_scheme, 0);
    rb_define_method(req_class, "rack_input", rack_input, 0);
    rb_define_method(req_class, "rack_errors", rack_errors, 0);
    rb_define_method(req_class, "rack_multithread", rack_multithread, 0);
    rb_define_method(req_class, "rack_multiprocess", rack_multiprocess, 0);
    rb_define_method(req_class, "rack_run_once", rack_run_once, 0);
    rb_define_method(req_class, "headers", headers, 0);
    rb_define_method(req_class, "body", body, 0);

    new_id = rb_intern("new");
    
    stringio_class = rb_const_get(rb_cObject, rb_intern("StringIO"));

    connect_val = rb_str_new_cstr("CONNECT");			rb_gc_register_address(&connect_val);
    content_length_val = rb_str_new_cstr("CONTENT_LENGTH");	rb_gc_register_address(&content_length_val);
    content_type_val = rb_str_new_cstr("CONTENT_TYPE");		rb_gc_register_address(&content_type_val);
    delete_val = rb_str_new_cstr("DELETE");			rb_gc_register_address(&delete_val);
    empty_val = rb_str_new_cstr("");				rb_gc_register_address(&empty_val);
    get_val = rb_str_new_cstr("GET");				rb_gc_register_address(&get_val);
    head_val = rb_str_new_cstr("HEAD");				rb_gc_register_address(&head_val);
    http_val = rb_str_new_cstr("http");				rb_gc_register_address(&http_val);
    options_val = rb_str_new_cstr("OPTIONS");			rb_gc_register_address(&options_val);
    path_info_val = rb_str_new_cstr("PATH_INFO");		rb_gc_register_address(&path_info_val);
    post_val = rb_str_new_cstr("POST");				rb_gc_register_address(&post_val);
    put_val = rb_str_new_cstr("PUT");				rb_gc_register_address(&put_val);
    query_string_val = rb_str_new_cstr("QUERY_STRING");		rb_gc_register_address(&query_string_val);
    rack_errors_val = rb_str_new_cstr("rack.errors");		rb_gc_register_address(&rack_errors_val);
    rack_input_val = rb_str_new_cstr("rack.input");		rb_gc_register_address(&rack_input_val);
    rack_multiprocess_val = rb_str_new_cstr("rack.multiprocess");rb_gc_register_address(&rack_multiprocess_val);
    rack_multithread_val = rb_str_new_cstr("rack.multithread");rb_gc_register_address(&rack_multithread_val);
    rack_run_once_val = rb_str_new_cstr("rack.run_once");	rb_gc_register_address(&rack_run_once_val);
    rack_url_scheme_val = rb_str_new_cstr("rack.url_scheme");	rb_gc_register_address(&rack_url_scheme_val);
    rack_version_val = rb_str_new_cstr("rack.version");		rb_gc_register_address(&rack_version_val);
    rack_version_val_val = rb_str_new_cstr("2.0.3");		rb_gc_register_address(&rack_version_val_val);
    request_method_val = rb_str_new_cstr("REQUEST_METHOD");	rb_gc_register_address(&request_method_val);
    script_name_val = rb_str_new_cstr("SCRIPT_NAME");		rb_gc_register_address(&script_name_val);
    server_name_val = rb_str_new_cstr("SERVER_NAME");		rb_gc_register_address(&server_name_val);
    server_port_val = rb_str_new_cstr("SERVER_PORT");		rb_gc_register_address(&server_port_val);
    slash_val = rb_str_new_cstr("/");				rb_gc_register_address(&slash_val);
}