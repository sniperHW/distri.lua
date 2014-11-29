<?php
header("cache-control:no-cache,must-revalidate");
header("Content-Type:text/html;charset=utf8");

function split_line($input,$separator){
	$ret = array();
	$line = strtok($input,$separator);
	while($line != ""){
		//echo $line . '<br>';
		array_push($ret,$line);
		$line = strtok($separator);
	}
	return $ret;
}
$redis = new Redis();
$redis->connect('127.0.0.1', 6379);
$machine_status = $redis->get('machine');
$machine_status = base64_decode($machine_status);
$lines = split_line($machine_status,"\n");
while($line = each($lines)){
	echo $line['value'] . '<br>';
}
?>