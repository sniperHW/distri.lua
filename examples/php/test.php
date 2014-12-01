<!DOCTYPE html>
<html>
	<head>
		<link rel="stylesheet" href="./codebase/webix.css" type="text/css" media="screen" charset="utf-8">
		<script src="./codebase/webix.js" type="text/javascript" charset="utf-8"></script>
		<script type="text/javascript" src="./common/testdata.js"></script>
		<link rel="stylesheet" type="text/css" href="./common/samples.css">
		<style>
			#areaA, #areaB {
				margin: 50px 10px;
				width:320px;
				height:400px;
				float: left;
			}
			#areaC{
				margin: 50px 10px;
				width:800px;
				height:400px;
				float: left;			
			}
		</style>
		<title>Tabview</title>
	</head>
	<body>
		<script type="text/javascript">
		var xmlHttp;
		url_data = null;
		function createXMLHttpRequest(){
			if(window.ActiveXObject){
				xmlHttp = new ActiveXObject("Microsoft.XMLHTTP");
			}
			else if(window.XMLHttpRequest){
				xmlHttp = new XMLHttpRequest();
			}
		}
		function start(){
			createXMLHttpRequest();
			var url="top.php";
			xmlHttp.open("POST",url,true);
			xmlHttp.setRequestHeader("Content-Type","application/x-www-form-urlencoded; charset=UTF-8");		
			xmlHttp.onreadystatechange = callback;
			xmlHttp.send(url_data);
		}
		function callback(){
			if(xmlHttp.readyState == 4){
				if(xmlHttp.status == 200){
					document.getElementById("show").innerHTML = xmlHttp.responseText;
					setTimeout("start()",1000);
				}
			}
		}
		function refresh(){
			createXMLHttpRequest();
			var url="top.php";
			xmlHttp.open("POST",url,true);
			xmlHttp.setRequestHeader("Content-Type","application/x-www-form-urlencoded; charset=UTF-8");		
			xmlHttp.onreadystatechange = callback1;
			xmlHttp.send(url_data);
		}
		function callback1(){
			if(xmlHttp.readyState == 4){
				if(xmlHttp.status == 200){
					document.getElementById("show").innerHTML = xmlHttp.responseText;
				}
			}
		}		
		start();
		</script>	
		<div id="areaA"></div>
		<table border="0" id="show">
		</table>		
		<script type="text/javascript" charset="utf-8">
			selected_id = "0";
			var PhysicalView = 0;
			webix.ui({
				container: "areaA",
				borderless:true, 
				view:"tabview",
				cells:[
					{
						header:"Physical View",
						body:{
							select:true,
							view:"tree",
							ready:function(){ 
								PhysicalView = this;
							},							
							on:{
								"onAfterSelect":function(id){
									var item = this.getItem(id)
									if(item.type == "ip"){
										url_data = null;
										refresh();
									}else if(item.type == "process"){
										url_data = "process="+item.value;
										refresh();
									}
								}
							},		                			
							<?php
								$redis = new Redis();
								$redis->connect('127.0.0.1', 6379);
								$deployment = $redis->get('deployment');
								echo "data: webix.copy($deployment)";
							?>																	
						},
					},	
				]
			});
		</script>
	</body>
</html>