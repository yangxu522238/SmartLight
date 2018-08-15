package com.thundersoft.flagmingo.smog;

public class SmogController {
    private  static  SmogController instance;

    private SmogUIFragment mSmogFragment;


    public  static SmogController getInstance()
    {
        if(null==instance)
            instance=new SmogController();
        return instance;
    }

    public SmogUIFragment getSmogFragment()
    {
        return mSmogFragment;
    }

    private SmogController()
    {
        mSmogFragment = new SmogUIFragment();
    }
}
