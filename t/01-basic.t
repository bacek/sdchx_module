use Test::Nginx::Socket no_plan;
use Test::More;

my $servroot = $Test::Nginx::Socket::ServRoot;
$ENV{TEST_NGINX_SERVROOT} = $servroot;

add_block_preprocessor(sub {
    my $block = shift;
    $block->set_value('http_config',
      "
        client_body_temp_path $servroot/client_temp;
        proxy_temp_path $servroot/proxy_temp;
        fastcgi_temp_path $servroot/fastcgi_temp;
        uwsgi_temp_path $servroot/uwsgi_temp;
        scgi_temp_path $servroot/scgi_temp;
      ");
    return $block;
  });


repeat_each(2);
no_shuffle();
no_root_location();
run_tests();

__DATA__

=== TEST 1: Sanity
--- user_files
>>> sdch/js.dict
This is an Javascript dictionary
>>> sdch/test.js
This is an Javascript dictionary

--- config
location / {
  sdchx on;
  sdchx_dictionary static {
    url /sdch/js.dict;
    file $TEST_NGINX_SERVROOT/html/sdch/js.dict;
    tag js;
    algo vcdiff;
    max-age 3600;
  }
}
--- request
GET /sdch/js.dict HTTP/1.1
--- response_body
This is an Javascript dictionary
--- response_headers
! SDCHx-Server-Id

=== TEST 2: Link header advertised
--- user_files
>>> sdch/js.dict
This is an Javascript dictionary
>>> sdch/test.js
This is an Javascript dictionary

--- config
location / {
  sdchx on;
  sdchx_dictionary static {
    url /sdch/js.dict;
    file $TEST_NGINX_SERVROOT/html/sdch/js.dict;
    tag js;
    algo vcdiff;
    max-age 3600;
  }
}
--- request
GET /sdch/test.js HTTP/1.1
--- response_body
This is an Javascript dictionary
--- response_headers
! SDCHx-Server-Id
--- response_headers_like
Link: <\S+>; rel="sdchx-dictionary"


=== TEST 3: Dictionary headers
--- user_files
>>> sdch/js.dict
This is an Javascript dictionary
>>> sdch/test.js
This is an Javascript dictionary

--- config
location / {
  sdchx on;
  sdchx_dictionary static {
    url /sdch/js.dict;
    file $TEST_NGINX_SERVROOT/html/sdch/js.dict;
    tag js;
    algo vcdiff;
    max-age 3600;
  }
}
--- request
GET /sdch/js.dict HTTP/1.1
--- more_headers
Accept-Encoding: gzip, sdchx

--- response_body
This is an Javascript dictionary

--- response_headers
SDCHx-Algo: vcdiff
SDCHx-Tag: js
SDCHx-Server-Id: IhySF33OfOd6rHdQDTAlJvqwyUKokUkUdfr6N0ow_0c
Cache-Control: max-age=3600

=== TEST 4: Encoded with single dictionary
--- user_files
>>> sdch/js.dict
This is an Javascript dictionary
>>> sdch/test.js
This is an Javascript dictionary

--- config
location / {
  sdchx on;
  sdchx_dictionary static {
    url /sdch/js.dict;
    file $TEST_NGINX_SERVROOT/html/sdch/js.dict;
    tag js;
    algo vcdiff;
    max-age 3600;
  }
}
--- request
GET /sdch/test.js HTTP/1.1
--- more_headers
Accept-Encoding: gzip, sdchx
SDCHx-Avail-Dictionary: IhySF33O

--- error_code: 242
--- response_headers
Content-Encoding: sdchx
SDCHx-Algo: vcdiff
SDCHx-Used-Dictionary-Id: IhySF33OfOd6rHdQDTAlJvqwyUKokUkUdfr6N0ow_0c
! Content-Length
Transfer-Encoding: chunked
