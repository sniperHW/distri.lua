<?php

/*function http_request($url,$method='GET',$data='',$cookie='',$refer=''){
    $errstr = '';
    $errno = 0;
    $header='';
    $body='';
    $newcookie='';
    return "here1";
    if (preg_match('/^http:\/\/(.*?)(\/.*)$/',$url,$reg)){$host=$reg[1]; $path=$reg[2];}
    else {outs(1,"URL($url)格式非法!"); return "URL($url)格式非法!";}
    $http_host=$host;
    if (preg_match('/^(.*):(\d+)$/', $host, $reg)) {$host=$reg[1]; $port=$reg[2];}
    else $port=80;
    $fp = fsockopen($host, $port, $errno, $errstr, 30);
    if (!$fp) {
      outs(1,"$errstr ($errno)\n");
    } else {
      fputs($fp, "$method $path HTTP/1.1\r\n");
      fputs($fp, "Host: $http_host\r\n");
      if ($refer!='') fputs($fp, "Referer: $refer\r\n");
      if ($cookie!='') fputs($fp, "Cookie: $cookie\r\n");
      fputs($fp, "Content-type: application/x-www-form-urlencoded\r\n");
      fputs($fp, "Content-length: ".strlen($data)."\r\n");
      fputs($fp, "Connection: close\r\n\r\n");
      fputs($fp, $data . "\r\n\r\n");
      $header_body=0;
      $chunked_format=0;
      $chunked_len=0;
      while (!feof($fp)) {
          $str=fgets($fp);
          //$len=hexdec($str);        if ($header_body==1) {echo ">>$str\t$len\n";        $str=fread($fp,$len);echo $str;}
          if ($header_body==1){
            if ($chunked_format){
              if ($chunked_len<=0){
                $chunked_len=hexdec($str);
                if ($chunked_len==0) break;
                else continue;
              } else {
                $chunked_len-=strlen($str);
                if ($chunked_len<=0) $str=trim($str);
                //elseif ($chunked_len==0) fgets($fp);
              }
            }
            $body.=$str;
          }
          else if ($str=="\r\n") $header_body=1;
          else {
            $header.=$str;
            if ($str=="Transfer-Encoding: chunked\r\n") $chunked_format=1;
            if (preg_match('|Set-Cookie: (\S+)=(\S+);|',$str,$reg)) $newcookie.=($newcookie==''?'':'; ').$reg[1].'='.$reg[2];
          }
      }
      fclose($fp);
    }
    $GLOBALS['TRAFFIC']+=414+strlen($url)+strlen($data)+strlen($header)+strlen($body);
    if (preg_match('/^Location: (\S+)\r\n/m',$header,$reg)) {
      if (substr($reg[1],0,1)!='/'){
        $path=substr($path,0,strrpos($path,'/')+1);
        $path.=$reg[1];
      } else $path=$reg[1];
      if ($newcookie) $cookie=$newcookie;
      return http_request('http://'.$http_host.$path,'GET','',$cookie,$url);
    }
    return array($body, $header, $newcookie);
}

echo http_request('http://127.0.0.1:8800','GET','','','');*/
/**
* 远程打开URL
* @param string $url   打开的url，　如 http://www.baidu.com/123.htm
* @param int $limit   取返回的数据的长度
* @param string $post   要发送的 POST 数据，如uid=1&password=1234
* @param string $cookie 要模拟的 COOKIE 数据，如uid=123&auth=a2323sd2323
* @param string $ip   IP地址
* @param int $timeout   连接超时时间
* @param bool $block   是否为阻塞模式
* @return    取到的字符串
*/
function uc_fopen($url, $limit = 0, $post = '', $cookie = '', $ip = '', $timeout = 15, $block = TRUE) {
  echo '1';
  $return = '';
  $matches = parse_url($url);
  !isset($matches['host']) && $matches['host'] = '';
  !isset($matches['path']) && $matches['path'] = '';
  !isset($matches['query']) && $matches['query'] = '';
  !isset($matches['port']) && $matches['port'] = '';
  echo '2';
  $host = $matches['host'];
  $path = $matches['path'] ? $matches['path'].($matches['query'] ? '?'.$matches['query'] : '') : '/';
  $port = !empty($matches['port']) ? $matches['port'] : 80;
  echo '3';
  if($post) {
     echo '4';
     $out = "POST $path HTTP/1.0\r\n";
     $out .= "Accept: **\r\n";
     //$out .= "Referer: $boardurl\r\n";
     $out .= "Accept-Language: zh-cn\r\n";
     $out .= "User-Agent: $_SERVER[HTTP_USER_AGENT]\r\n";
     $out .= "Host: $host\r\n";
     $out .= "Connection: Close\r\n";
     $out .= "Cookie: $cookie\r\n\r\n";
  }else {
     echo '5';
     $out = "GET $path HTTP/1.0\r\n";
     $out .= "Accept: */*\r\n";
     //$out .= "Referer: $boardurl\r\n";
     $out .= "Accept-Language: zh-cn\r\n";
     $out .= "User-Agent: $_SERVER[HTTP_USER_AGENT]\r\n";
     $out .= "Host: $host\r\n";
     $out .= "Connection: Close\r\n";
     $out .= "Cookie: $cookie\r\n\r\n";
  }
  echo '6';
  $fp = @fsockopen(($ip ? $ip : $host), $port, $errno, $errstr, $timeout);
  if(!$fp) {
     return '';//note $errstr : $errno \r\n
  } else {
     stream_set_blocking($fp, $block);
     stream_set_timeout($fp, $timeout);
     @fwrite($fp, $out);   
     $status = stream_get_meta_data($fp);
      echo '9';
      @fclose($fp);
      return;
      $stop = false;     
     if(!$status['timed_out']) {
      while (!feof($fp)) {
       if(($header = @fgets($fp)) && ($header == "\r\n" || $header == "\n")) {
        break;
       }
      }
      while(!feof($fp) && !$stop) {
       $data = fread($fp, ($limit == 0 || $limit > 8192 ? 8192 : $limit));
       $return .= $data;
       if($limit) {
        $limit -= strlen($data);
        $stop = $limit <= 0;
       }
      }
     }
     @fclose($fp);
     return $return;
  }
}



function SendMsg2Daemon($ip,$port,$msg,$timeout = 15){
  if(!$ip || !$port || !$msg){
    return array(false);
  }
  $errno;
  $errstr;
  $fp = @fsockopen($ip,$port,$errno,$errstr,$timeout);
  if(!$fp){
    return array(false);
  }
  stream_set_blocking($fp, true);
  stream_set_timeout($fp, $timeout);
  @fwrite($fp, $msg . "\n");   
  $status = stream_get_meta_data($fp);
  $ret;
  if(!$status['timed_out']) {
      $datas = 'data:';
      while(!feof($fp)){
        $data = fread($fp, 4096);
        if($data){
          $datas = $datas . $data;
        }
      }
      return array(true,$datas);
  }else{
    $ret = array(false);
  }
  @fclose($fp);
  return ret;  
}

$result = SendMsg2Daemon('127.0.0.1','8800',$_POST['op']);
if($result[0]){
  echo $result[1]; 
}else{
  echo 'SendMsg2Daemon error!';
}
?>