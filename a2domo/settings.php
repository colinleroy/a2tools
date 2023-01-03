<?php
  $token="****************************************";

  $ha_root_url = "http://homecontrol.lan:8123";
  $hc_root_url = "http://homecontrol.lan";
  $max_num_lines = 450;

  $list = [
    /* Must return CSV with switch_id;switch_name;switch_state */
    "switches" => [
      "token_header" => "Authorization: Bearer $token",
      "content_type" => "application/json",
      "endpoint" => "$hc_root_url/csv/switches.php",
    ],

    /* Must return CSV with zone_id;zone_name;zone_set_temperature;zone_cur_temperature;zone_cur_humidity */
    "climate" => [
      "token_header" => "Authorization: Bearer $token",
      "content_type" => "application/json",
      "caps" => "CAN_AWAY,CAN_SCHEDULE", /* comma-separated list, can contain CAN_AWAY and CAN_SCHEDULE */
      "full_home_zone" => "-1", /* optionally specify zone id for full home */
      "hot_water_zone" => "0", /* optionally specify zone id for hot water setting */
      "endpoint" => "$hc_root_url/csv/tado_zones.php",
    ],

    /* Must return CVS with sensor_id;sensor_name;num_days;sensor_cur_value;sensor_unit */
    "sensors" => [
      "token_header" => "Authorization: Bearer $token",
      "content_type" => "application/json",
      "endpoint" => "$ha_root_url/api/template",
      "body" => json_encode(["template" => "{%set sensors = states|selectattr('entity_id', 'search', '^sensor\\..*(_circuit_power|_circuit_total|eau.*_total|envoy_[a-z].*power|gaz_kwh_total|gaz_power|linky|otgw_)') %}
{% for item in sensors -%}
{{item.entity_id}};{{item.attributes.friendly_name}};{%if item.attributes.unit_of_measurement|default('?')|regex_search('[Lh]$') %}30{% else %}1{% endif %};{{item.state|default('0')|int(0)}};{{item.attributes.unit_of_measurement|default('?')}}
{% endfor %}"])
    ],

    /* Must return CSV with timestamp;value */
    "sensor_metrics" => [
      "token_header" => "Authorization: Bearer $token",
      "content_type" => "application/json",
      "date_format" => "Y-m-d\TH:iP",
      "endpoint" => "$ha_root_url/api/history/period/{START_DATE}?end_time={END_DATE}&filter_entity_id={SENSOR_ID}",
      "filter" => "ha_sensor_metrics_filter.php"
    ],

  ];

  $control = [
    /* Must return CSV with switch_id;switch_name;switch_state */
    "switches" => [
      "token_header" => "Authorization: Bearer $token",
      "content_type" => "application/json",
      "endpoint" => "$hc_root_url/control/toggle_switch.php?switch_number={SWITCH_ID}",
    ],

    /* Must return CSV with zone_id;zone_name;zone_set_temperature;zone_cur_temperature;zone_cur_humidity */
    "climate" => [
      "token_header" => "Authorization: Bearer $token",
      "content_type" => "application/json",
      "endpoint" => "$hc_root_url//control/toggle_tado.php?id={ZONE_ID}&state={PRESENCE}&override={MODE}&temp={SET_TEMP}",
    ],

  ]

?>
