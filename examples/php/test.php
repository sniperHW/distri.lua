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
			xmlHttp.open("GET",url,true);
			xmlHttp.onreadystatechange = callback;
			xmlHttp.send(null);
		}
		function callback(){
			if(xmlHttp.readyState == 4){
				if(xmlHttp.status == 200){
					document.getElementById("show").innerHTML = xmlHttp.responseText;
					setTimeout("start()",1000);
				}
			}
		}
		start();
		</script>	
		<div id="areaA"></div>
		<div id="areaC" style="background:black">
		<p><font color="white"><span id="show"></span></font></p>
		</div>		
		<script type="text/javascript" charset="utf-8">
			last_select = "0";
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
			                			activeTitle:true,
							on:{
								"onAfterSelect":function(id){
									webix.message(last_select);
									last_select = id;
								}
							},		                			
							<?php
								$redis = new Redis();
								$redis->connect('127.0.0.1', 6379);
								$deployment = $redis->get('deployment');
								echo "data: webix.copy($deployment)";
							?>																	
						}
					},
				]
			});
		</script>
	</body>
</html>