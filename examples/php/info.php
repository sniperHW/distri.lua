<?php
header("cache-control:no-cache,must-revalidate");
header("Content-Type:text/html;charset=utf8");

function split_line($input,$separator){
	$ret = array();
	$line = strtok($input,$separator);
	while($line != ""){
		array_push($ret,$line);
		$line = strtok($separator);
	}
	return $ret;
}
$redis = new Redis();
$redis->connect('127.0.0.1', 6379);
$machine_status = $redis->get('machine');
$machine_status = base64_decode($machine_status);
$process_info = $redis->get('process');
$process_info = base64_decode($process_info);
$deployment = $redis->get('deployment');
echo "[$machine_status ,$process_info,$deployment ]";
//echo $machine_status;
?>

