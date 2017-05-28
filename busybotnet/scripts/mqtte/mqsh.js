
//mqsh.js
// depends on mqtt, colors, readcommand, corporal
// async shell for infinite number of clients
// todo: fix things, hacky v1 beta release, bad argument option handling
//	- also	need to add file transfer function for bbn update?
// 		~ kod ~
//
//
//HOW TO INSTALL:   $ mkdir mqsh; mv mqsh.js mqsh; cd mqsh; npm install optimist; npm install mqtt; npm install corporal; npm install colors; npm install readcommand; echo done; nodejs mqsh.js localhost

//var sleep = require('sleep');
// set up our dependancies
var mqtt    = require('mqtt');
var Corporal = require('corporal');
var colors = require("colors/safe");
var readcommand = require('readcommand');
var argv = require('optimist').argv;
//function for beginning a mqtt connection to a server, listening to topic defined by subtopic and publishing to pubtopic
function shell ()
{
	//this is where we connect
        var client  = mqtt.connect('mqtt://' + servername);
        console.log("mqtt.connect " + servername);

	//on client connection, we subscribe to the subtopic
	client.on('connect', function ()
	{
		client.subscribe(subtopic);
		//  client.publish('lol', 'f');
	});

	//on receiving a message we write a newline and then the message
	client.on('message', function (topic, message)
	{
		// message is Buffer
		console.log("");
			console.log(colors.yellow(message.toString()));
		//  client.end();
	});

	//for handling control c
	var sigints = 0;
	readcommand.loop(function(err, args, str, next)
	{
		if (err && err.code !== 'SIGINT')
		{
			throw err;
		}
		else if (err)
		{
			if (sigints === 1)
			{
				//	callback();
				process.exit(0);
					;
			}
			else
			{
				sigints++;
					console.log('Press ^C again to exit.');
					return next();
			}
		}
		else
		{
			sigints = 0;
		}
		
		// handle the input and send to pubtopic
		
		//console.log('Received args: %s', JSON.stringify(args));

		client.publish(pubtopic, args.join(" "));

			return next();
	});

}

//function to handle the initial tui
var corporal = new Corporal(
{
	//define out tui commands
	'commands':
	{
		//this needs to be improved, currently cannot fun a callback without prompt rendering for whatever reason. once launched you will have to re enter if you need to come back to tui
		'sh':
		{
			'description': 'open shell',
				'invoke': function(session, args, callback)
			{
				//session.stdout().write(args[0] + '\n');
				console.log(colors.blue(" [*] ") + colors.red("opening mqtt shell on ") + colors.green(servername));
					shell();
				//		callback();
			}
		},
			//hello world example
			'echo':
		{
			'description': 'echo',
				'invoke': function(session, args, callback)
			{
				session.stdout().write(args[0] + '\n');
					callback();
			}
		},
			//get currently selected environment variables
			'server':
		{
			'description': 'current server',
				'invoke': function(session, args, callback)
			{
				//session.stdout().write(args[0] + '\n');
				console.log(servername);
					callback();
			}
		},
                        'subtopic':
                {
                        'description': 'current subscription topic',
                                'invoke': function(session, args, callback)
                        {
                                //session.stdout().write(args[0] + '\n');
                                console.log(subtopic);
                                        callback();
                        }
                },
                        'pubtopic':
                {
                        'description': 'current publish topic',
                                'invoke': function(session, args, callback)
                        {
                                //session.stdout().write(args[0] + '\n');
                                console.log(pubtopic);
                                        callback();
                        }
                },
			//not working correctly, currently being called manually by shell()
                        'connect':
                {
                        'description': 'connect to current server',
                                'invoke': function(session, args, callback)
                        {
                                //session.stdout().write(args[0] + '\n');
                                console.log(servername);
					startlistener();
                                        callback();
                        }
                },
			//functions to change variables
                        'chpub':
                {
                        'description': 'change pubtopic',
                                'invoke': function(session, args, callback)
                        {
                                        console.log(colors.yellow('changing publish topic to ') + colors.magenta(args));
                                        pubtopic = args;
                                        callback();
                        }
                },
                        'chsub':
                {
                        'description': 'change subtopic',
                                'invoke': function(session, args, callback)
                        {
                                        console.log(colors.yellow('changing subscription topic to ') + colors.magenta(args));
                                	subtopic = args;
					callback();
                        }
                },


			'chserv': 
		{
			'description': 'change mqtt server',
				'invoke': function(session, args, callback)
			{
				console.log(colors.yellow('changing mqtt server to ') + colors.magenta(args));
				servername = args;
				callback();
			}			
		}

	}
});
//function to start tui
function startinteractive () 
{
	corporal.on('load', corporal.loop);
}

	// set up variables for use in shell data pipe
	var subtopic = 'data'
	var pubtopic = 'shell'

	// take in argument from cmdline as servername for mqtt connection
	var servername = 'localhost';
	//old style server argument passing
//	var procargs = process.argv.slice(2);
//	if (process.argv[2] != null || process.argv[2] != undefined) {
//	        var servername = procargs;
//	}

	//new style
	if (argv.h)
	{
		servername = argv.h;
	}
	if (argv.s) {
		shell();	
	}
	else {
		console.log('welcome to mqsh type help to begin');
		startinteractive();
	}

