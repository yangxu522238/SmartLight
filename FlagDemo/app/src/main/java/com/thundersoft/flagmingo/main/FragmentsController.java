package com.thundersoft.flagmingo.main;

import android.app.Activity;
import android.app.Fragment;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.widget.TextView;

import com.ashokvarma.bottomnavigation.BottomNavigationBar;
import com.ashokvarma.bottomnavigation.BottomNavigationItem;
import com.thundersoft.flagmingo.curtain.CurtainController;
import com.thundersoft.flagmingo.curtain.CurtainUIFragment;
import com.thundersoft.flagmingo.light.LightController;
import com.thundersoft.flagmingo.light.LightUIFragment;
import com.thundersoft.flagmingo.smog.SmogController;
import com.thundersoft.flagmingo.smog.SmogUIFragment;

public class FragmentsController implements BottomNavigationBar.OnTabSelectedListener {

    private static final String TAG = FragmentsController.class.getSimpleName();

    private static  final  String CURTAIN_TITLE= "CURTAIN";
    private static  final  String LIGHT_TITLE= "LIGHT";
    private static  final  String SMOG_INTITLE= "SMOG";

    private static Activity mainActivity;
    public static  Activity getMainActivity()
    {
        return mainActivity;
    }

    private TextView mTitleTextView;

    private BottomNavigationBar mBottomNavigationBar;
    private FragmentManager mFragmentManager;
    private FragmentTransaction mFragmentTransaction;

    private CurtainUIFragment mCurtainUIFragment;
    private SmogUIFragment mSmogUIFragment;
    private LightUIFragment mLightUIFragment;

    private Fragment mLastFragment;

    public FragmentsController(Activity activity)
    {
        mainActivity=activity;
        mFragmentManager = mainActivity.getFragmentManager();
        mTitleTextView = activity.findViewById(R.id.main_title);
        mCurtainUIFragment = CurtainController.getInstance().getCurtainUIFragment();
        mSmogUIFragment = SmogController.getInstance().getSmogFragment();
        mLightUIFragment = LightController.getInstance().getLightUIFragment();
    }

    public void CreatBottomNavigationBar()
    {
        mBottomNavigationBar = mainActivity.findViewById(R.id.bottom_navigation_bar);
        mBottomNavigationBar
                .addItem(new BottomNavigationItem(R.drawable.ic_curtain_white_24dp, "Curtain"))
                .addItem(new BottomNavigationItem(R.drawable.ic_light_white_24dp, "Ligth"))
                .addItem(new BottomNavigationItem(R.drawable.ic_smog_white_24dp, "Smog"))
                .setFirstSelectedPosition(0)
                .initialise();
        mBottomNavigationBar.setTabSelectedListener(this);
        InitAllFragment();
        setDefaultFragment();
    }

    private void InitAllFragment()
    {
        mFragmentTransaction = mFragmentManager.beginTransaction();
        mFragmentTransaction.add(R.id.tb, mCurtainUIFragment).add(R.id.tb,mLightUIFragment).add(R.id.tb,mSmogUIFragment);
        mFragmentTransaction.commit();
    }

    private void setDefaultFragment() {
        mFragmentTransaction = mFragmentManager.beginTransaction();
        mLastFragment = mCurtainUIFragment;
        mFragmentTransaction.show(mCurtainUIFragment);
        mFragmentTransaction.commit();
        mTitleTextView.setText(CURTAIN_TITLE);
    }

    @Override
    public void onTabSelected(int position) {
        mFragmentTransaction = mFragmentManager.beginTransaction();
        mFragmentTransaction.hide(mLastFragment);
        switch (position) {
            case 0:
                mLastFragment=mCurtainUIFragment;
                mTitleTextView.setText(CURTAIN_TITLE);
                break;
            case 1:
                mLastFragment=mLightUIFragment;
                mTitleTextView.setText(LIGHT_TITLE);
                break;
            case 2:
                mLastFragment=mSmogUIFragment;
                mTitleTextView.setText(SMOG_INTITLE);
                break;
        }
        mFragmentTransaction.show(mLastFragment);
        mFragmentTransaction.commit();
    }

    @Override
    public void onTabUnselected(int position) {

    }

    @Override
    public void onTabReselected(int position) {

    }
}
