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

    header("Content-Type: text/csv");
    $curl = curl_init();

    curl_setopt($curl, CURLOPT_URL, $list[$item_type]["endpoint"]);
    curl_setopt($curl, CURLOPT_RETURNTRANSFER, true);

    if (isset($list[$item_type]["body"]) && $list[$item_type]["body"] != NULL) {
      curl_setopt($curl, CURLOPT_CUSTOMREQUEST, "POST");
      curl_setopt($curl, CURLOPT_POSTFIELDS, $list[$item_type]["body"]);
    }

    $headers = array();
    $headers_set = false;

    if (isset($list[$item_type]["token_header"]) && $list[$item_type]["token_header"] != NULL) {
      $headers[] = $list[$item_type]["token_header"];
      $headers_set = true;
    }
    if (isset($list[$item_type]["content_type"]) && $list[$item_type]["content_type"] != NULL) {
      $headers[] = "Content-Type: ".$list[$item_type]["content_type"];
      $headers_set = true;
    }

    if ($headers_set) {
      curl_setopt($curl, CURLOPT_HTTPHEADER, $headers);
    }

    $response = curl_exec($curl);
    if (!json_decode($response))
        echo iconv("UTF-8", "ISO646-FR1//TRANSLIT", $response);
    else
	echo $response;
?>
