<?php
  require_once("settings.php");
  $json = json_decode($response);

  $group = -1;
  if (preg_match('/.*[hL]$/',$unit)) {
    $group = ($scale > 4*86400) ? 86400 : 3600;
  }

  $num_lines = 0;
  $last_time = -1;

  foreach ($json[0] as $item) {
    $ts = date_create_from_format('Y-m-d\TH:i:s.uP', $item->last_changed);
    if ($ts !== false)
      $timestamp = date_timestamp_get($ts);
    if ($ts !== false && $item->state !== "unknown" && $item->state !== "" && $item->state !== "unavailable") {
      if ($timestamp - $last_time >= $group) {
        $num_lines++;
        $last_time = $timestamp;
      }
    }
  }
  $modulo = 1;
  if ($num_lines > $max_num_lines) {
    $modulo = (int)($num_lines / $max_num_lines);
  }

  $last_time = -1;
  $last_val = 0;
  $num_lines = 0;
  foreach ($json[0] as $item) {
    $ts = date_create_from_format('Y-m-d\TH:i:s.uP', $item->last_changed);
    if ($ts !== false)
      $timestamp = date_timestamp_get($ts);
    if ($ts !== false && $item->state !== "unknown" && $item->state !== "" && $item->state !== "unavailable") {
      if ($timestamp - $last_time >= $group) {
        if ($num_lines++ % $modulo == 0) {
          if ($last_val == 0) {
            $last_val = $item->state;
          }
          if ($group > -1) {
            $val = round($item->state - $last_val, 2);
          } else {
            $val = $item->state;
          }
          echo $timestamp.';'.$val."\n";
          $last_time = $timestamp;
          $last_val = $item->state;
        }
      }
    }
  }
?>
