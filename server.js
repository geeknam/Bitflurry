var sys = require("sys"),
    http = require("http"),
    exec = require('child_process').exec,
	url = require("url");
	
http.createServer(function(request, response) {
	var uri = url.parse(request.url).pathname;  
	var filename = uri.substring(1);
    response.sendHeader(200, {"Content-Type": "text/plain"});
	child = exec("/root/FYP/bitflurry get "+ filename +" - 2>&1", function (error, stdout, stderr) {
		response.write(stdout);
	});
    response.close();
}).listen(8080);

sys.puts("Server running at http://localhost:8080/");
