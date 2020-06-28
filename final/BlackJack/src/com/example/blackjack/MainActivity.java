package com.example.blackjack;

import android.app.Activity;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;

public class MainActivity extends Activity {
	public native int driverOpen();
	public native int driverClose(int fd);
	public native int[] getDealerData(int fd);
	private static final String TAG = "MYTAG";
	Handler msgHandler = new Handler(){
		public void handleMessage(Message msg){
			if(msg.what==0){
				/* set text using argument msg */
				if(msg.arg1 == -1){	// End thread (end program)
					moneytv.setText(" ");
					cardstv.setText("Bye! :D");
					statustv.setText(" ");
				}
				else{	// Continued thread (in Game)
				/*Show dealer card and money*/
				moneytv.setText("Money : "+msg.arg1);
				cardstv.setText("Cards : "+msg.obj);
				String status_msg = "";
				/* message for descript detail */
				switch(msg.arg2){
				case 0:
					status_msg ="Please Home button to start game! ";
					break;
				case 1:
					status_msg = "playing! press UP to HIT, press DOWN to STAND";
					break;
				case 2:
					status_msg = "player win! player get 100$! :) \nPlease press HOME button to continue the game. ";
					break;
				case 3:
					status_msg = "BUST! player lost 200$! :( \nPlease press HOME button to continue the game.";
					break;
				case 4:
					status_msg = "Dealer win! player lost 100$ \nPlease press HOME button to continue the game.";
					break;
				case 5:
					status_msg = "BLACK JACK!! player get 200$ \n Please press HOME button to continue the game.";
					break;
				case 6:
					status_msg = "Player lost all money! :( Press HOME to continue!";
					break;
				}
				statustv.setText(status_msg);
				}
			}
		}
	};
	
	class DriverThread extends Thread{
		int money = 1000;
		int point = 0;
		int[] dealer = new int [20];
		int fd, status;
		String cards = "";
		Handler driverHandler;
		
		DriverThread(Handler handler){
			driverHandler = handler;
		}
		
		public void run(){
			fd = driverOpen();
			while(true){
				Message msg = Message.obtain();
				cards = "";
				dealer = getDealerData(fd);
				point = dealer[0];
				money = dealer[2];
				status = dealer[3];
				Log.v(TAG,"dealer idx:"+dealer[1]);
				if(status == 1){
					/*only show one card*/
					String to = Integer.toString(dealer[4]);
					cards = cards.concat(to+" (hidden)");
				}
				else{
					/*show all card*/
					for(int i=0; i<dealer[1] ;i++){
						String to = Integer.toString(dealer[i+4]);
						cards = cards.concat(to+" ");
					}
				}
				msg.arg1 = money;
				msg.arg2 = status;
				msg.obj = cards;
				driverHandler.sendMessage(msg);
				if(point < 0) break; //end game (when press BACK)
			}
			Message msg = Message.obtain();
			msg.arg1 = -1;
			msg.arg2 = -1;
			msg.obj = " ";
			driverHandler.sendMessage(msg);
			driverClose(fd);
		}	
	}
	
	TextView titletv;
	TextView cardstv;
	TextView moneytv;
	TextView statustv;
	Button btn;
	MediaPlayer mp;
	OnClickListener ltn;
	DriverThread driverThread;
	int mp_cnt =0;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		System.loadLibrary("blackjack");
		/* set views */		
		setContentView(R.layout.activity_main);
		titletv = (TextView)findViewById(R.id.titlevalue);
		titletv.setText("BlackJack Dealer");
		moneytv= (TextView)findViewById(R.id.moneyvalue);
		cardstv= (TextView)findViewById(R.id.cardsvalue);
		statustv= (TextView)findViewById(R.id.statusvalue);
		cardstv.setText("WELCOME!");
		statustv.setText("Please press HOME to start game!");
		btn = (Button)findViewById(R.id.btn);
		
		/* music event (TURN ON/OFF) by button */
		mp=MediaPlayer.create(this, R.raw.bgmusic);
		mp.start();
		ltn = new OnClickListener(){
			public void onClick(View v){
				mp_cnt++;
				if(mp_cnt%2==0)
					mp.start();
				else
					mp.pause();
			}
		};
		btn.setOnClickListener(ltn);
		
		/*thread for jni*/
		driverThread = new DriverThread(msgHandler);
		driverThread.setDaemon(true);
		driverThread.start();
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}


}
