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
run_tests();

__DATA__

=== TEST 1: Sanity
--- user_files
>>> sdch/js.dict
This is an Javascript dictionary

--- config
location /sdch {
  sdchx on;
  sdchx_dictionary static {
    url /sdch/js.dict;
    file $TEST_NGINX_SERVROOT/html/sdch/js.dict;
    tag js;
    algo vcdiff;
    max-age 3600;
  }

  # return 200 "FOO";
}
--- request
GET /sdch/js.dict HTTP/1.1
--- response_body
This is an Javascript dictionary

