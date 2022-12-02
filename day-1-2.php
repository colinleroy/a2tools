#!/usr/bin/php
<?php
$sum = 0;
$sums = [];

while($line = fgets(STDIN)) {
    $val=trim($line);
    if ($val != "") {
        $sum += intval($val);
    } else {
        $sums[] = $sum;
        $sum = 0;
    }
}
rsort($sums);
$top_three = array_sum(array_slice($sums, 0, 3));
echo "top three sum: ".$top_three."\n";

?>
