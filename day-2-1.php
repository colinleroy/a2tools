#!/usr/bin/php
<?php

function play_score($opponent, $me) {
  /* Matrix of score given opponent shape / my shape */
  $score_matrix = [/*       X  Y  Z */
                   /* A */ [3, 6, 0],
                   /* B */ [0, 3, 6],
                   /* C */ [6, 0, 3]
                  ];

  /* convert opponent shapes A-C to 0-2 index */
  $opponent = ord($opponent) - ord('A');

  /* convert my shapes X-Z to 0-2 index */
  $me = ord($me) - ord('X');
  
  $shape_score = $me + 1;
  $round_score = $score_matrix[$opponent][$me];

  return $shape_score + $round_score;
}

$sum = 0;

while($line = trim(fgets(STDIN))) {
    $moves = explode(" ", $line, 2);
    $sum += play_score($moves[0], $moves[1]);
}
echo "Score: ".$sum."\n";

?>
