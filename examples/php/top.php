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
$process = $_POST['process'];

echo  "<tr align=\"left\">";
if($process){
	$flag = false;
	$finded = false;
	while($line = each($lines)){
		if(!$flag){
			if($line['value'] == "process_info")
				$flag = true;
		}else{
			if(strstr($line['value'],$process)){ 
				$finded = true;
			}
		}
	}
	if($finded){
		echo  "<td><input type=\"button\" value=\"Stop\" /><input type=\"button\" value=\"Kill\" /></td>";
	}else{
		echo  "<td><input type=\"button\" value=\"Start\" /></td>";
	}	
}else{
	echo  "<td><input type=\"button\" value=\"Start\" /><input type=\"button\" value=\"Stop\" /><input type=\"button\" value=\"Kill\" /></td>";
}
echo  "</tr>";		
echo   "<tr>";	
echo   "<td align=\"left\">";		
echo "<div id=\"areaC\" style=\"background:black\">";
echo "<p><font color=\"white\">";

reset($lines);
if($process){
	$flag = false;
	while($line = each($lines)){
		if(!$flag){
			echo $line['value'] . '<br>';
			if($line['value'] == "process_info")
				$flag = true;
		}else{
			if(strstr($line['value'],$process)){ 
				echo $line['value'] . '<br>';
			}
		}
	}	
}else{
	while($line = each($lines)){
		echo $line['value'] . '<br>';
	}
}
echo "</font></p></div></td></tr>";
?>