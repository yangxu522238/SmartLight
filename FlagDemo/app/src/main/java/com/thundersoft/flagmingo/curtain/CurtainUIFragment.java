package com.thundersoft.flagmingo.curtain;

import android.app.Activity;
import android.app.DatePickerDialog;
import android.app.Fragment;
import android.app.TimePickerDialog;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.DatePicker;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.TimePicker;
import android.widget.Toast;

import com.thundersoft.flagmingo.main.FragmentsController;
import com.thundersoft.flagmingo.main.R;
import com.thundersoft.wilddog.Wilddog;
import com.wilddog.client.SyncReference;
import com.wilddog.client.WilddogSync;

import java.text.DateFormat;
import java.util.Calendar;
import java.util.Locale;


public class CurtainUIFragment extends Fragment implements View.OnClickListener {

    private Activity mActivity;
    private Button mAddMissionBtn;
    private LinearLayout mMissionSettingPlane;
    private Button mSetDateBtn;
    private Button mSetTimeBtn;
    private Button mComfirmBtn;
    private Button mCancelBtn;

    private SeekBar mMissionSettingSlider;
    private SeekBar mCurtainControlSlider;

    private TextView mMissionDateTip;
    private TextView mMissionTimeTip;
    private TextView mMissionValueTip;

    private ListView mMissionListView;

    DateFormat fmtDate=DateFormat.getDateInstance();
    DateFormat fmtTime=DateFormat.getTimeInstance();
    Calendar dateAndTime = Calendar.getInstance(Locale.CHINA);

    DatePickerDialog mDatePickerDialog=null;
    TimePickerDialog mTimePickerDialog=null;

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mActivity = FragmentsController.getMainActivity();
        CurtainController.getInstance().AddWilddogListeners();

    }

    @Nullable
    @Override
        public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, Bundle savedInstanceState) {
        View view=inflater.inflate(R.layout.curtain_fragment,container,false);
        mAddMissionBtn=(Button) view.findViewById(R.id.add_mission_btn);
        mSetDateBtn =(Button)view.findViewById(R.id.mission_set_date_btn);
        mSetTimeBtn=(Button)view.findViewById(R.id.mission_set_time_btn);
        mComfirmBtn=(Button)view.findViewById(R.id.mission_comfirm_btn);
        mCancelBtn=(Button)view.findViewById(R.id.mission_cancel_btn);
        mMissionDateTip=(TextView)view.findViewById(R.id.mission_date_tip);
        mMissionTimeTip=(TextView)view.findViewById(R.id.mission_time_tip);
        mMissionValueTip=(TextView)view.findViewById(R.id.curtion_misson_slider_tip);
        mMissionSettingSlider = (SeekBar)view.findViewById(R.id.curtain_mission_slider);
        mCurtainControlSlider=(SeekBar)view.findViewById(R.id.control_curtain_slider);
        mMissionListView= (ListView)view.findViewById(R.id.mission_list_view);
        mMissionSettingPlane =(LinearLayout)view.findViewById(R.id.mission_plane);
        mMissionListView.setAdapter(CurtainController.getInstance().getMissionAdapter());
        mMissionSettingSlider.setOnSeekBarChangeListener(missionSliderListener);
        mCurtainControlSlider.setOnSeekBarChangeListener(curtainControlSliderListener);
        mComfirmBtn.setOnClickListener(this);
        mCancelBtn.setOnClickListener(this);
        mSetTimeBtn.setOnClickListener(this);
        mSetDateBtn.setOnClickListener(this);
        mAddMissionBtn.setOnClickListener(this);
        InitDateAndTimePickerDialog();
        return  view;
    }

    @Override
    public void onClick(View v) {
        switch (v.getId())
        {
            case R.id.add_mission_btn:
                if(mMissionSettingPlane.getVisibility()==View.GONE)
                    mMissionSettingPlane.setVisibility(View.VISIBLE);
                Wilddog wilddog= Wilddog.getInstance();
                SyncReference sync = WilddogSync.getInstance().getReference("/Flagmingo/smoke");
                sync.setValue("108");
                break;
            case R.id.mission_set_date_btn:
                mDatePickerDialog.show();
                Wilddog wilddogl= Wilddog.getInstance();
                SyncReference syncl = WilddogSync.getInstance().getReference("/Flagmingo/smoke");
                syncl.setValue("1");
                break;
            case R.id.mission_set_time_btn:
                mTimePickerDialog.show();
                break;
            case R.id.mission_comfirm_btn:
                long systemTime= System.currentTimeMillis();
                long missionTime = dateAndTime.getTimeInMillis();
                if(missionTime-systemTime<60)
                    Toast.makeText(mActivity,"Date time setting is error, please re-enter",Toast.LENGTH_SHORT).show();
                else{
                    if(CurtainDate.mCurtainMissionListKey.contains(missionTime+""))
                        Toast.makeText(mActivity,"Date time setting is repeated, please re-enter",Toast.LENGTH_SHORT).show();
                    else
                         {
                            Mission mission=new Mission(CurtainDate.mSimpleDateFormat.format(dateAndTime.getTime()), mMissionSettingSlider.getProgress()+"");
                            CurtainController.getInstance().AddCurtainMissionList(missionTime+"",mission);
                            HideMissionSettingPlane();
                        }
                }
                break;
            case R.id.mission_cancel_btn:
                HideMissionSettingPlane();
                break;
        }
    }

    public void HideMissionSettingPlane()
    {
        mMissionSettingPlane.setVisibility(View.GONE);
    }

    public void UpdateCurtainControlSliderValue(String value)
    {
        mCurtainControlSlider.setProgress(Integer.parseInt(value));
    }

    private void InitDateAndTimePickerDialog()
    {
        mDatePickerDialog=  new DatePickerDialog(mActivity,
                d,
                dateAndTime.get(Calendar.YEAR),
                dateAndTime.get(Calendar.MONTH),
                dateAndTime.get(Calendar.DAY_OF_MONTH));
        mDatePickerDialog.setCanceledOnTouchOutside(false);
        mTimePickerDialog= new TimePickerDialog(mActivity,
                t,
                dateAndTime.get(Calendar.HOUR_OF_DAY),
                dateAndTime.get(Calendar.MINUTE),
                false);
        mTimePickerDialog.setCanceledOnTouchOutside(false);
    }

    private DatePickerDialog.OnDateSetListener d = new DatePickerDialog.OnDateSetListener()
    {
        @Override
        public void onDateSet(DatePicker view, int year, int monthOfYear,
                              int dayOfMonth) {
            dateAndTime.set(Calendar.YEAR, year);
            dateAndTime.set(Calendar.MONTH, monthOfYear);
            dateAndTime.set(Calendar.DAY_OF_MONTH, dayOfMonth);
            mMissionDateTip.setText(fmtDate.format(dateAndTime.getTime()));
        }
    };

    private TimePickerDialog.OnTimeSetListener t = new TimePickerDialog.OnTimeSetListener() {
        @Override
        public void onTimeSet(TimePicker view, int hourOfDay, int minute) {
            dateAndTime.set(Calendar.HOUR_OF_DAY, hourOfDay);
            dateAndTime.set(Calendar.MINUTE, minute);
            mMissionTimeTip.setText(fmtTime.format(dateAndTime.getTime()));
        }
    };


    private SeekBar.OnSeekBarChangeListener missionSliderListener = new SeekBar.OnSeekBarChangeListener() {
        @Override
        public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
           mMissionValueTip.setText(""+progress);
        }

        @Override
        public void onStartTrackingTouch(SeekBar seekBar) {

        }

        @Override
        public void onStopTrackingTouch(SeekBar seekBar) {

        }
    };
    private SeekBar.OnSeekBarChangeListener curtainControlSliderListener=new SeekBar.OnSeekBarChangeListener() {
        @Override
        public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {

        }

        @Override
        public void onStartTrackingTouch(SeekBar seekBar) {

        }

        @Override
        public void onStopTrackingTouch(SeekBar seekBar) {
            CurtainController.getInstance().ChangeCurtainValue(seekBar.getProgress());
        }
    };


}
