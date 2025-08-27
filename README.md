
# WEBSERV
a little webserver made from scratch

## About
webserv is a small server based on the http 1.1 and is inspired by nginx
\
it support: 
- server location
-  get / post / delete http request
- auto indexing
- redirection
- python / php cgi
- error pages
- cookies / session
- uploading

## Usage
### request
this is the request you can do:
> [!TIP]
> `<ip>` e.g: http://localhost:8080 \
> `<loc>` e.g: http://localhost:8080/foo (loc is always optional)

|name|description|
|----|----------|
|curl -X POST -H "request_type:dir_content" `<ip>/<loc>`| list the content of the directory|
|curl -X POST -H "request_type:upload" --data "test" `<ip>/<loc>`| upload `data` to of the directory|
|curl -X DELETE `<ip>/<loc>/foo`| delete the file foo (`DELETE` methode need to be enable in the location) |
|curl -X GET `<ip>/<loc>/foo`| get the content of foo |
|curl -X POST -H "request_type:client_credentials" `<ip>` | get the `<UID>` associated to your ip |
|curl -X POST -H "request_type:client_visits" -H "UID:`<UID>`" `<ip>`|return the number of request of `<UID>`|


> webserv will store the id of an ip and the number of request it made into a session file

### Configuration
webserv will need a configuration file called `<name>.conf`


> [!IMPORTANT]
> all path are absolute


-- directives for the server section

| name | description | 
|------|-------------|
|server|define the begining of a new server|
|listen `<port>`| which port is the server listening|
|server_name `<name>` | name of the server|
| root `<path>`| root directory of the server |
|error_page `<number>` | set the error page linked to `<number>`
|client_max_body_size `<number>`| define the max size of a response in bytes |
| location `<name>` | define the begining of new location|
| cookies | define the begining of a cookies section|

-- directives for the location section
> [!TIP]
> the location `foo` will be use when the has `foo` in it e.g: http://localhost:8080/foo/index.html 

| name | description |
|------|-------------|
|index `<path>`| default html page|
|path `<path>` | override the root of the server, uri will be append to it|
|allowed_methods `<methodes>`| which methodes are allowed on the location|
|upload_dir `<path>`|where does uploded file go|
|cgi_pass `<path>`| executable to send the cgi (only work if loc is cgi)|
|directory_listing `<on/off>`| enable / disable the directory listing (same as auto index on nginx) | \


> [!TIP]
> how to use cgi \
> change: ```location `<name>``` \
> to: ```location ~ <extension>```

-- cookies directives
| name | description |
|------|-------------|
| set `<cookie>` | set a new cookie|

### example configuration 
```
server {
	root /var/www/html
	listen 8080

	server_name webserv
	client_max_body_size 10M

	cookies {
		set name=super_weberv
		set superValue=8
	}

	location / {
		path /var/www/html
		allowed_methods GET POST
	}

	location /aliexpress.fr {
		return 301 https://www.amazon.fr/
	}

	location ~ .php {
		cgi_pass /bin/php-cgi
	}

	location ~ .py {
		cgi_pass /bin/python3
	}

	error_page 403 /errors_pages/403.html
	error_page 404 /errors_pages/404.html
	error_page 405 /errors_pages/405.html
	error_page 413 /errors_pages/413.html
	error_page 500 /errors_pages/500.html
	error_page 501 /errors_pages/501.html
	error_page 504 /errors_pages/504.html
}

```

> you will find other default conf file in /conf/<name> of the repo, these are:
- default.conf
- pretty.conf
- listing.conf

## Installation
clone the repo
``` bash
git clone https://github.com/abidolet/Webserv.git webserv && cd webserv
```

then build it using make
``` bash
make
```

to launch don't forget your config file
``` bash
./webserv <path-to-conf>
```
## Contributors
[@abidolet](https://github.com/Alexis42lyon)

[@ygille](https://github.com/Bluesmoothie)

[@mjuncker](https://github.com/Maxime-juncker)

