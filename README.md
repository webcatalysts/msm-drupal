# MongoDB Schema Manager MSM

## Installation

Install like a normal contrib module. And then install and start the node.js service. After installing in Drupal you can check the status of the node.js service at: /admin/structure/msm/status.

## Configure Node.js Service & MongoDB

From the command line, start the node.js service using either of the following:

1. Move into the msm directory (if you're not already there):
> `cd [/path/to/]msm/srv`

2. Copy _config-example.js_ to _config.js_ (if _config.js_ does not exist).
3. Edit _config.js_:

```
'use strict'

module.exports = {
  port: 3000,
  db: {
    uri: 'mongodb://127.0.0.1:27017',
  }
}
```
**Important Note**

Currently, to use a port other than 3000 for the node.js service, you must also change the Drupal setting with either of the following:

* With Drush: `drush vset msm_host "http://127.0.0.1:1234"`
* With PHP: `variable_set('msm_host', 'http://127.0.0.1:1234');`
* In settings.php:

```
$conf['msm_host'] = 'http://127.0.0.1:1234';
```

## Start the Node.js Service

From the command line, start the node.js service using either of the following:

1. Move into the msm directory (if you're not already there):
> `cd [/path/to/]msm/srv`

2. Install dependencies with
> `npm install`

3. Start the node.js serveri:
> `npm start` or `npm start-dev`[^startdev]

[^startdev]: Uses `npm start-dev` to restart the service after detecting file changes.

