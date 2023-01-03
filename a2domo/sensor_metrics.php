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
    $unit = $_REQUEST["unit"];
    $item_type = "sensor_metrics";

    if (strstr($list["sensor_metrics"]["endpoint"], "{SENSOR_ID}")) {
      $list["sensor_metrics"]["endpoint"] = str_replace("{SENSOR_ID}", $sensor_id, $list["sensor_metrics"]["endpoint"]);
    }
    if(isset($_REQUEST["scale"])) {
      $scale = $_REQUEST["scale"] * 86400;
      $start_date = date($list["sensor_metrics"]["date_format"], strtotime("today midnight") - $scale);
      $end_date = date($list["sensor_metrics"]["date_format"], strtotime("today midnight"));
    } else {
      $scale = 86400;
      $start_date = date($list["sensor_metrics"]["date_format"], strtotime("-".$scale." seconds"));
      $end_date = date($list["sensor_metrics"]["date_format"], strtotime("now"));
    }
    
    $list["sensor_metrics"]["endpoint"] = str_replace("{START_DATE}", urlencode($start_date), $list["sensor_metrics"]["endpoint"]);
    $list["sensor_metrics"]["endpoint"] = str_replace("{END_DATE}", urlencode($end_date), $list["sensor_metrics"]["endpoint"]);
    
    ob_start();
    include("list.php");
    $response = ob_get_clean();
    if (isset($list["sensor_metrics"]["filter"])) {
      include($list["sensor_metrics"]["filter"]);
    }
?>
