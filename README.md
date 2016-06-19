# mod-myvhost
**mod\_myvhost** is [Apache](http://httpd.apache.org/) module for dynamically configured name based mass [virtual hosting](http://en.wikipedia.org/wiki/Virtual_hosting) with PHP, virtual host's configurations and [PHP](http://php.net/) settings are stored in database.

No need to have every vhost in apache's configuration file, no need to restart apache after configuration changed (vhost added|deleted, vhost rootdir changed, vhost unblocked/unblocked...).

After all, it is capable to change settings of PHP4/5 dynamically (if php is loadable module or it is linked with apache) for any vhost. By default, it sets open\_basedir as vhost's rootdir to prevent user from stoling files from other users and from your server, but you have ability to change almost any parameter that exists in php.ini, for example, you can turn on safe\_mode or register\_globals for particular vhost, if it has old php scripts that use global variables.

### Copyright

  Copyright (c) 2005-2016 Igor Popov

License
-------
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

### Authors

  Igor Popov
  (ipopovi |at| gmail |dot| com)
