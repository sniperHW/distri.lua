<!DOCTYPE html>
<html>
	<head>
		<link rel="stylesheet" href="./codebase/webix.css" type="text/css" media="screen" charset="utf-8">
		<script src="./codebase/webix.js" type="text/javascript" charset="utf-8"></script>
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
		<style type="text/css">
			.webix_tree_error{
				background-image: url(./myicons/error.png)
			}
		</style>		
		<title>Tabview</title>
	</head>
	<body>
		<script type="text/javascript">
		</script>	
		<div id="areaA"></div>
		<table>
		<tr><input id = "Start" type="button" value="Start" /><input id = "Stop" type="button" value="Stop" /><input id = "Kill" type="button" value="Kill" /></tr>
		<tr><td>
			<div id = "areaC" style="background:black">
			<p><font color="white"><span id="processInfo"></span></font></p>
			</div>
		</td></tr>
		<tr><p><font color="black"><span id="ServiceDesp"></span></font></p></tr>		
		</table>		
		<script type="text/javascript" charset="utf-8">
			var filter = null;
			var xmlHttp = null;
			var treedata = null;
			var selected_id = null;
			var PhysicalView = null;
			var LogicalView = null;
			var machineInfo = null;
			var processInfo = null;
			var phyInit = false;
			var logInit = false;						
			function createXMLHttpRequest(){
				if(window.ActiveXObject){
					xmlHttp = new ActiveXObject("Microsoft.XMLHTTP");
				}
				else if(window.XMLHttpRequest){
					xmlHttp = new XMLHttpRequest();
				}
			}
			function fetchdata(){
				createXMLHttpRequest();
				var url="info.php";
				xmlHttp.open("GET",url,true);
				xmlHttp.setRequestHeader("Content-Type","application/x-www-form-urlencoded; charset=UTF-8");		
				xmlHttp.onreadystatechange = callback;
				xmlHttp.send(null);
			}
			function FindProcess(name){
				for(var i = 0,len = processInfo.length; i < len; i++){
					var s = String(processInfo[i].cmd);
					if(s.search(name) > 0){
						return true;									
					}
				}
				return false;
			}
			function buildPyhView(){
				var phyTree = {};
				for(var i = 0,len1 = treedata.length; i < len1; i++){
					var tmp1 = treedata[i].service;
					for(var j = 0,len2 = tmp1.length; j < len2; j++){
						var tmp2 = phyTree[tmp1[j].ip];
						if(!tmp2){
							tmp2 = [];
							phyTree[tmp1[j].ip] = tmp2;
						}
						tmp2.push(tmp1[j]);	
					}		
				}
				for (var key in phyTree){
					var services = phyTree[key];
					var rootitem = {value:key,type:"ip"};
					var rootid = PhysicalView.add(rootitem, null, 0);
					var err = false;					
					for(var i = 0,len = services.length; i < len; i++){
						var item;
						var icon = "";
						if(!FindProcess(services[i].type)){
							icon = "error";
							err = true;
						}
						if(services[i].type == "ssdb-server"){
							item = {value:services[i].type,type:"process",ip:services[i].ip,conf:services[i].conf,icon:icon};
						}else{
							item = {value:services[i].type,type:"process",ip:services[i].ip,port:services[i].port,icon:icon};
						}
						var id = PhysicalView.add(item, null, rootid);
					}
					if(err){
						PhysicalView.updateItem(rootid, {value:key,type:"ip",icon:"error"});
					}
				}
			}
			function buildLogView(){
				for(var i = 0,len1 = treedata.length; i < len1; i++){
					var tmp1 = treedata[i].service;
					var rootitem = {value:treedata[i].groupname,type:"group"};
					var rootid = LogicalView.add(rootitem, null, 0);
					var err = false;						
					for(var j = 0,len2 = tmp1.length; j < len2; j++){
						var item; 
						var icon = "";
						if(!FindProcess(tmp1[i].type)){
							icon = "error";
							err = true;							
						}						
						if(tmp1[i].type == "ssdb-server"){
							item = {value:tmp1[j].type,type:"process",ip:tmp1[j].ip,conf:tmp1[j].conf,icon:icon};
						}else{
							item = {value:tmp1[j].type,type:"process",ip:tmp1[j].ip,port:tmp1[j].port,icon:icon};
						}						
						var id = LogicalView.add(item, null, rootid);					
					}
					if(err){
						LogicalView.updateItem(rootid, {value:treedata[i].groupname,type:"group",icon:"error"});
					}								
				}
			}			
			function callback(){
				if(xmlHttp.readyState == 4){
					if(xmlHttp.status == 200){
						var info = JSON.parse(xmlHttp.responseText);
						var newtreedata = info[2];
						machineInfo = info[0];
						processInfo = info[1];
						var str = '';
						for(var i = 0,len = machineInfo.length; i < len; i++){
							str = str + machineInfo[i] + '</br>';
						}
						str = str + "-------------------------------process------------------------------- </br>";
						for(var i = 0,len = processInfo.length; i < len; i++){
							str = str + "pid:" + processInfo[i].pid + ",usr:" + processInfo[i].usr;
							str = str + ",cpu:" + processInfo[i].cpu + ",mem:" + processInfo[i].mem;
							str = str + ",cmd:" + processInfo[i].cmd + "</br>";
						}													
						/*var find = false;
						var gotmatch = false;
						if(filter == null){
							document.getElementById("Start").disabled = false;
							document.getElementById("Stop").disabled = false;
							document.getElementById("Kill").disabled = false;
							for(var i = 0,len = info.length; i < len; i++){
								str = str + info[i] + '</br>'
							}
						}else{
							for(var i = 0,len = info.length; i < len; i++){
								if(find == false){ 
									str = str + info[i] + '</br>';
									if(info[i] == 'process_info'){
										find = true;
									}
								}else{
									var s = String(info[i]);
									if(s.search(filter) > 0){
										str = str + info[i] + '</br>';
										gotmatch = true;										
									}
								}
							}
							if(gotmatch){
								document.getElementById("Start").disabled = true;
								document.getElementById("Stop").disabled = false;
								document.getElementById("Kill").disabled = false;
							}else{
								document.getElementById("Start").disabled = false;
								document.getElementById("Stop").disabled = true;
								document.getElementById("Kill").disabled = true;
							}														
						}*/						
						document.getElementById("processInfo").innerHTML = str;
						//webix.message("ok here1");
						if(!treedata){
							//alert(newtreedata.length);
							treedata = newtreedata;
							buildPyhView();
							buildLogView();
						}
						//webix.message("ok here3");						
						setTimeout("fetchdata()",1000);
					}
				}
			}						
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
								phyInit = true;
								if(logInit){
									fetchdata();
								}
							},							
							on:{
								"onAfterSelect":function(id){
									var item = this.getItem(id);
									var str = "";
									if(item.type == "ip"){
										filter = null;
									}else if(item.type == "process"){
										filter = item.value;
										if(item.value == "ssdb-server"){
											str = str + item.value + ":" + item.ip + ":" + item.conf;	
										}else{
											str = str + item.value + ":" + item.ip + ":" + item.port;	
										}
									}
									document.getElementById("ServiceDesp").innerHTML = str;
									fetchdata();
								}
							},
							data:[]
						},
					},
					{
						header:"Logical View",
						body:{
							select:true,
							view:"tree",
							ready:function(){ 
								LogicalView = this;
								logInit = true;
								if(phyInit){
									fetchdata();
								}								
							},							
							on:{
								"onAfterSelect":function(id){
									var item = this.getItem(id);
									var str = "";
									if(item.type == "ip"){
										filter = null;
									}else if(item.type == "process"){
										filter = item.value;
										if(item.value == "ssdb-server"){
											str = str + item.value + ":" + item.ip + ":" + item.conf;	
										}else{
											str = str + item.value + ":" + item.ip + ":" + item.port;	
										}
									}
									document.getElementById("ServiceDesp").innerHTML = str;
									fetchdata();
								}
							},
							data:[]
						},
					},							
				]
			});
		</script>
	</body>
</html>