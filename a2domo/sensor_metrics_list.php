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
    $sensor_id = $_REQUEST["sensor_id"];
    $item_type = "sensor_metrics_list";
    if (strstr($list["sensor_metrics_list"]["endpoint"], "{SENSOR_ID}")) {
      $list["sensor_metrics_list"]["endpoint"] = str_replace("{SENSOR_ID}", $sensor_id, $list["sensor_metrics_list"]["endpoint"]);
    }
    include("list.php");
?>
