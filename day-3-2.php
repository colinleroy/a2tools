#!/usr/bin/php
<?php

function find_char($lines, $index) {
    for ($i = 0; $i < strlen($lines[$index]); $i++) {
        if (strstr($lines[$index + 1], $lines[$index][$i]) > -1
         && strstr($lines[$index + 2], $lines[$index][$i]) > -1) {
             return $lines[$index][$i];
         }
    }
    die("did not find anything at index $index\n");
}

$lines = file("inputs/3.txt", FILE_IGNORE_NEW_LINES);

$sum = 0;
for ($i = 0; $i + 2 < sizeof($lines); $i +=3) {
    $char = find_char($lines, $i);

    $val = ord($char);
    if ($val > 96) {
        $prio = $val - 96;
    } else {
        $prio = $val - 38;
    }
    $sum += $prio;
}
echo "Sum: $sum\n";

?>
