function JSONrequest(method, url, done_cb, error_cb, data) {
    var request = new XMLHttpRequest();
    request.open(method, url, true);
    request.setRequestHeader('Content-type', 'application/json');
    request.onreadystatechange = function() {
		if (request.readyState == 4 && request.status == 204) {
			done_cb();
		}
		else if (request.readyState == 4 && request.status > 199 && request.status < 400) {
			var result = JSON.parse(request.responseText);
			done_cb(result);
		}
		else if (request.readyState == 4) {
			if (error_cb !== undefined) {
				error_cb();
			}
		}
	}
	request.send(data);
}
