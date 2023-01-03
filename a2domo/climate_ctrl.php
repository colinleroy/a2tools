<?php /*
 Copyright (C) 2018 Colin Leroy <colin@colino.net>
 This file is part of homecontrol <https://git.colino.net/>.

 homecontrol is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 homecontrol is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with homecontrol.  If not, see <http://www.gnu.org/licenses/>.
*/ ?>
<?php
    require_once("settings.php");
    $item_type = "climate";
    $zone_id = $_REQUEST["id"];
    $set_temp = $_REQUEST["set_temp"];
    $presence = $_REQUEST["presence"];
    $mode = $_REQUEST["mode"];

    $control["climate"]["endpoint"] = str_replace("{ZONE_ID}", $zone_id, $control["climate"]["endpoint"]);
    if (isset($control["climate"]["body"]))
      $control["climate"]["body"] = str_replace("{ZONE_ID}", $zone_id, $control["climate"]["body"]);

    $control["climate"]["endpoint"] = str_replace("{SET_TEMP}", $set_temp, $control["climate"]["endpoint"]);
    if (isset($control["climate"]["body"]))
      $control["climate"]["body"] = str_replace("{SET_TEMP}", $set_temp, $control["climate"]["body"]);

    $control["climate"]["endpoint"] = str_replace("{PRESENCE}", $presence, $control["climate"]["endpoint"]);
    if (isset($control["climate"]["body"]))
      $control["climate"]["body"] = str_replace("{PRESENCE}", $presence, $control["climate"]["body"]);

    $control["climate"]["endpoint"] = str_replace("{MODE}", $mode, $control["climate"]["endpoint"]);
    if (isset($control["climate"]["body"]))
      $control["climate"]["body"] = str_replace("{MODE}", $mode, $control["climate"]["body"]);

    include("control.php");
?>
