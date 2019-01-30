class JsonRPC{
	constructor(url)
	{
		this.urlsocket = "";
		if (location.protocol === "http:")
			this.urlsocket = "ws://";
		else if (location.protocol === "https:")
			this.urlsocket = "wss://";
		this.urlsocket += location.hostname;
		this.urlsocket += ":"+location.port;
		this.urlsocket += "/"+url;
		this.id = 0;
		this.timer = 0;
		this.wsready = false;
	}
	connect()
	{
		if (this.timer)
			clearTimeout(this.timer);
		this.websocket=new WebSocket(this.urlsocket);
		
		this.websocket.onopen = function(evt) {
			this.wsready = true;
			if (typeof(this.onopen) == "function")
				this.onopen.call(this);
		}.bind(this);
		this.websocket.onmessage = this.receive.bind(this);
		this.websocket.onerror = function(evt) {console.log("error")};
		this.websocket.onclose = this.reconnect.bind(this);
	}

	receive(evt)
	{
		var data=JSON.parse(evt.data);
		console.log(data);
		if (typeof(this.onmessage) == "function")
			this.onmessage.call(this, data);
	}

	reconnect()
	{
		this.wsready = false;
		if (typeof(this.onopen) == "function")
			this.onclose.call(this);
		this.timer = setTimeout(this.connect.bind(this), 3000);
	}

	send(method, params)
	{
		var paramsstr;
		if (typeof(params) == "object")
			paramsstr = JSON.stringify(params);
		else
			paramsstr = params
		var msg = '{"jsonrpc":"2.0","method":"'+method.toString()+'","params":'+paramsstr+',"id":'+this.id+'}';
		if (this.wsready)
		{
			this.websocket.send(msg);
			this.id++;
		}
	}
}
