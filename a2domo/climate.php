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
    echo "CAPS;".$list["climate"]["caps"]."\n";
    echo "FULL_HOME_ZONE;".$list["climate"]["full_home_zone"]."\n";
    echo "HOT_WATER_ZONE;".$list["climate"]["hot_water_zone"]."\n";
    include("list.php");
?>
