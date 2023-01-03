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
    $item_type = "switches";
    $switch_id = $_REQUEST["id"];
    
    $control["switches"]["endpoint"] = str_replace("{SWITCH_ID}", $switch_id, $control["switches"]["endpoint"]);
    if (isset($control["switches"]["body"]))
      $control["switches"]["body"] = str_replace("{SWITCH_ID}", $switch_id, $control["switches"]["body"]);
    
    include("control.php");
?>
