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

    //header("Content-Type: text/csv");
    $curl = curl_init();

    curl_setopt($curl, CURLOPT_URL, $control[$item_type]["endpoint"]);
    curl_setopt($curl, CURLOPT_RETURNTRANSFER, true);

    if (isset($control[$item_type]["body"]) && $control[$item_type]["body"] != NULL) {
      curl_setopt($curl, CURLOPT_CUSTOMREQUEST, "POST");
      curl_setopt($curl, CURLOPT_POSTFIELDS, $control[$item_type]["body"]);
    }

    $headers = array();
    $headers_set = false;

    if (isset($control[$item_type]["token_header"]) && $control[$item_type]["token_header"] != NULL) {
      $headers[] = $control[$item_type]["token_header"];
      $headers_set = true;
    }
    if (isset($control[$item_type]["content_type"]) && $control[$item_type]["content_type"] != NULL) {
      $headers[] = "Content-Type: ".$control[$item_type]["content_type"];
      $headers_set = true;
    }

    if ($headers_set) {
      curl_setopt($curl, CURLOPT_HTTPHEADER, $headers);
    }

    $response = curl_exec($curl);
    echo $response;
?>
