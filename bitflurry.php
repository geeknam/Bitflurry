<?php
//setlocale(LC_CTYPE, "UTF8", "en_GB.UTF-8");

set_time_limit(0);
$media = $_GET['f'];

$ext = substr($media, strrpos($media, '.') + 1);
$content_type = NULL;

switch ($ext) {
    case "avi":
        $content_type = "video/x-msvideo";
        break;
    case "mp3":
        $content_type = "audio/mp3";
        break;
}


if (!$media || $media == "") die("Specify a filename");
if (!isset($content_type)) die("File format not supported");

header("Content-type: ".$content_type);

chdir("/root/FYP");
$media = str_replace(" ", "\ ", $media);

//passthru("/root/FYP/bitflurry get ".$media." -");

$handle = popen("/root/FYP/bitflurry get ".$media." - 2>&1", "rb");

while (!feof($handle)) {
        $read = fread($handle, 4096);
        echo $read;
        ob_flush();
}
pclose($handle);

?>

