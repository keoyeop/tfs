/**
 * 
 */
package com.taobao.tfstest;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Iterator;
import java.util.Map.Entry;
import java.util.Date;

import com.taobao.gaia.AppCluster;
import com.taobao.gaia.AppGrid;
import com.taobao.gaia.AppServer;
import com.taobao.gaia.HelpConf;
import com.taobao.gaia.HelpFile;
import com.taobao.gaia.HelpHA;
import com.taobao.gaia.HelpLog;
import com.taobao.gaia.HelpProc;
import com.taobao.gaia.KillTypeEnum;

import org.apache.log4j.Logger;
import org.springframework.context.ApplicationContext;
import org.springframework.context.support.ClassPathXmlApplicationContext;

public class NameServerPlanTestCase {
	final ApplicationContext beanFactory = new ClassPathXmlApplicationContext("tfsServer.xml");
	final AppGrid tfsGrid 		= (AppGrid) beanFactory.getBean("tfsGrid");
	//final AppGrid slaveTfsGrid 	= (AppGrid) beanFactory.getBean("slaveTfsGrid");
	
	protected static Logger log = Logger.getLogger("TfsTest");
	
	public HelpConf conf = new HelpConf();
	public HelpHA HA = new HelpHA();
	public HelpLog Log = new HelpLog();
	public HelpProc Proc = new HelpProc();
	public HelpFile File = new HelpFile();
	
	final ApplicationContext clientFactory = new ClassPathXmlApplicationContext("tfsClient.xml");
	
	final AppServer tfsSeedClient = (AppServer) clientFactory.getBean("seedClient");
	final AppServer tfsReadClient = (AppServer) clientFactory.getBean("readClient");
	final AppServer tfsUnlinkClient = (AppServer) clientFactory.getBean("unlinkClient");
	//final AppServer tfsReadClient_slave = (AppServer) clientFactory.getBean("slaveReadClient");
	
	//Define
	final public int NSINDEX = 0;
	final public int DSINDEX = 1;
	final public int DSINDEXI = 2;
	final public int FAILCOUNTNOR = 0;
	final public String NSVIP = tfsGrid.getCluster(NSINDEX).getServer(0).getVip();
	final public String NSIPA = tfsGrid.getCluster(NSINDEX).getServer(0).getIp();
	final public String NSIPB = tfsGrid.getCluster(NSINDEX).getServer(1).getIp();
	final public int NSPORT = tfsGrid.getCluster(NSINDEX).getServer(0).getPort();
	final public String NSRES = tfsGrid.getCluster(NSINDEX).getServer(0).getResName();
	final public String NSMACA = tfsGrid.getCluster(NSINDEX).getServer(0).getMacName();
	final public String NSMACB = tfsGrid.getCluster(NSINDEX).getServer(1).getMacName();
	final public String IPALIAS = tfsGrid.getCluster(NSINDEX).getServer(0).getIpAlias();
	final public String NSCONF = tfsGrid.getCluster(NSINDEX).getServer(0).getConfname();	
	final public String DSIPA = tfsGrid.getCluster(DSINDEX).getServer(0).getIp();
	final public String DSIPB = tfsGrid.getCluster(DSINDEXI).getServer(1).getIp();
	final public String CLIENTIP = tfsSeedClient.getIp();
	final public String CLIENTCONF = tfsSeedClient.getConfname();
	
	/* Path */
	final public String TFS_HOME        = tfsGrid.getCluster(NSINDEX).getServer(0).getDir(); 
	final public String TEST_HOME       = tfsSeedClient.getDir(); 
	final public String TFS_LOG_HOME    = TFS_HOME + "/logs";
	final public String TFS_BIN_HOME    = TFS_HOME + "/bin";
	final public String NS_LOG_NAME     = TFS_LOG_HOME + "/nameserver.log";
	final public String DP_LOG_NAME     = TFS_LOG_HOME + "/dumpplan.log";
	final public String BL_LOG_NAME     = TFS_LOG_HOME + "/blocklist.log";
	final public String CURR_LOG_NAME   = "nameserver.log";	
	public String SAR_DSA_LOG     		= TFS_HOME + "/sar_";	
	public String SAR_DSB_LOG     		= TFS_HOME + "/sar_";
	
	/* Key word */
	final public String WRITEFILE        = "writeFile :";
	final public String READFILE         = "readFile :";
	final public String UNLINKFILE         = "unlinkFile :";
	final public String SAVEUNIQUEFILE   = "SaveUniqueFile :";
	final public String UNIQUE           = "Unique :";
	final public String WRITEFILESTATIS  = "write statis:";
	final public String SAVEUNIQUESTATIS = "saveUnique statis:";
	final public String UNIQUESTATIS     = "uniqueFile statis:";
	final public String READFILESTATIS   = "read statis:";
	final public String UNLINKSTATIS   	 = "unlink statis:";
	final public String PLANSEQNO        = "plan seqno";
	final public String REPLICATE        = "replicate";
	final public String MOVE             = "move";
	final public String COMPACT          = "compact";
	final public String DELETE           = "delete";
	final public String STATUS_FINISH    = "status: finish";
	final public String STATUS_BEGIN     = "status: begin";
	final public String EMERGENCY        = "emergency";
	final public String NORMAL           = "normal";

	/* Plan type */
	enum PlanType 
	{
		PLAN_TYPE_REPLICATE,
		PLAN_TYPE_EMERG_REPLICATE,
		PLAN_TYPE_MOVE,
		PLAN_TYPE_COMPACT,
		PLAN_TYPE_DELETE
	};
	
	/* Column numbers */
	/* block_id'col number in dumpplan.log(processed) */
	final public int BLOCKID_DP_COL      = 13;
	/* priority'col number in dumpplan.log(processed) */
	final public int PRIORITY_DP_COL      = 11;
	/* block_id'col number in blockList.log(generated by ssm) */
	final public int BLOCKID_BL_COL      = 1;
	/* copynum'col number in blockList.log(generated by ssm) */
	final public int COPYNUM_BL_COL      = 8;
	/* delfile'col number in blockList.log(generated by ssm) */
	final public int DELFILE_BL_COL      = 5;
	/* dsip'col number in serverList.log(generated by ssm) */
	final public int DSIP_SL_COL      	 = 1;
	/* blkcnt'col number in serverList.log(generated by ssm) */
	final public int BLKCNT_SL_COL       = 5;
	/* rxbyt/s'col number in sar_ds.log(generated by sar) */
	final public int RXBYTPERSEC_SD_COL  = 5;
	/* txbyt/s'col number in sar_ds.log(generated by sar) */
	final public int TXBYTPERSEC_SD_COL  = 6;
	/* used_cap'col number in server list(ssm) */
	final public int USED_CAP_COL        = 3;
	/* curr_blk_cnt'col number in server list(ssm) */
	final public int CURR_BLKCNT_COL     = 6;
	
	/* Define blk cnt and network traffic diff threshold to check write balance */
	final public int   BLK_DIFF_THRESHOLD       		  = 10;	
	final public double NETWORKTRAF_IMBALANCE_THRESHOLD   = 1000000.0;
	/* Define used capacity threshold for check in write test(GB)*/
	final public double CHK_WRITE_AMOUNT       		      = 400.0;	
	/* Define blk cnt for check in write test*/
	final public int BLK_CNT_THRESHOLD	                  = 40000;	
	
	/* Client conf */
	final public String LOOPFLAG = "loop_flag";
	final public String FILESIZE = "writeCount";
	final public String UNITSIZEMIN = "unit_min";
	final public String UNITSIZEMAX = "unit_max";
	final public String READTYPE = "largeFlag";
	final public String UNLINKRATIO = "unlinkRatio";
	final public int LOOPON = 1;
	final public int LOOPOFF = 0;
	
	final public String WRITECMD = "./tfsControlPress -f test_tfs.conf -i tfsSeed -l tfsSeed.";
	final public String READCMD = "./tfsControlPress -f test_tfs.conf -i tfsRead -l tfsRead.";
	final public String UNLINKCMD = "./tfsControlPress -f test_tfs.conf -i tfsUnlink -l tfsUnlink.";
	
	/* For scan log on client */
	final public int TAILLINE = 100000;
	final public int TAILRATECOL = 12;
	final public int RUNRATECOL = 14;
	final public int TAILTPSCOL = 13;
	final public int RUNTPSCOL = 12;
	final public int SCANTIME = 120;
	
	/* Thread count on client */
	final public int HIGHTHREAD = 100;
	final public int LOWTHREAD = 10;
	final public int READTHREAD = 1;
	
	/* The write mode on client */
	final public int WRITEONLY = 1;
	final public int WRITEUNI = 2;
	final public int WRITEMIX = 3;
	final public int READ = 4;
	final public int TEMP = 8;
	final public int READSLAVE = 16;
	final public int UNLINK = 32;
	
	/* Standard */
	final public float SUCCESSRATE = 95;
	final public float HIGHRATE = (float) 99.99;
	final public float HALFRATE = 50;
	final public float FAILRATE = 0;
	final public int WAITTIME = 30;
	final public int BLOCK_CHK_TIME = 500;
	final public int PLAN_CHK_NUM   = 20;
	final public int TEST_TIME = 5400;     //in second
	final public int SAMPLE_INTERVAL = 60;  //in second(sar)
	
	/* Other */
	public String caseName = "";
	
	/* Vip */
	public String masterIp = NSIPA;
	
	/**
	 * 
	 * @param sec
	 */
	public void sleep(int sec)
	{
		log.debug("wait for "+sec+"s");
		try
		{
			Thread.sleep(sec*1000);
		}
		catch(Exception e)
		{
			e.printStackTrace();
		}
	}
	
	/**
	 * 
	 * @param logName
	 * @return
	 */
	public boolean writeCmd()
	{
		boolean bRet = false;
		log.info("Write command start ===>");
		bRet = Proc.proStartBackroundBase(CLIENTIP, WRITECMD + caseName, TEST_HOME);
		log.info("Write command end ===>");
		return bRet;
	}
	
	/**
	 * 
	 * @param logName
	 * @return
	 */
	public boolean writeCmdStop()
	{
		boolean bRet = false;
		log.info("Write command stop start ===>");
		bRet = Proc.proStopByCmd(CLIENTIP, WRITECMD + caseName);
		log.info("Write command stop end ===>");
		return bRet;
	}
	
	/**
	 * 
	 * @param logName
	 * @return
	 */
	public boolean readCmd()
	{
		boolean bRet = false;
		log.info("Read command start ===>");
		bRet = Proc.proStartBackroundBase(CLIENTIP, READCMD + caseName, TEST_HOME);
		log.info("Read command end ===>");
		return bRet;
	}
	
	/**
	 * 
	 * @param logName
	 * @return
	 */
	public boolean readCmdStop()
	{
		boolean bRet = false;
		log.info("Write command stop start ===>");
		bRet = Proc.proStopByCmd(CLIENTIP, READCMD + caseName);
		log.info("Write command stop end ===>");
		return bRet;
	}
	
	public boolean readCmdMon()
	{
		boolean bRet = false;
		log.info("Read command monitor start ===>");
		for (;;)
		{
			int iRet = Proc.proMonitorBase(CLIENTIP, READCMD + caseName);
			if (iRet == 0)
			{
				bRet = true;
				break;
			} else if (iRet > 0)
			{
				continue;
			} else {
				bRet = false;
				break;
			}
		}
		log.info("Read command monitor end ===>");
		return bRet;
	}
	
	/**
	 * 
	 * @param logName
	 * @return
	 */
	public boolean unlinkCmd()
	{
		boolean bRet = false;
		log.info("Unlink command start ==>");
		bRet = Proc.proStartBackroundBase(CLIENTIP, UNLINKCMD + caseName, TEST_HOME);
		log.info("Unlink command end ==>");
		return bRet;
	}
	
	public boolean unlinkCmdMon()
	{
		boolean bRet = false;
		log.info("Read command monitor start ===>");
		for (;;)
		{
			int iRet = Proc.proMonitorBase(CLIENTIP, UNLINKCMD + caseName);
			if (iRet == 0)
			{
				bRet = true;
				break;
			} else if (iRet > 0)
			{
				continue;
			} else {
				bRet = false;
				break;
			}
		}
		log.info("Read command monitor end ===>");
		return bRet;
	}
	
	/**
	 * 
	 * @param iSize
	 * @return
	 */
	public boolean setSeedSize(int iSize)
	{
		boolean bRet = false;
		bRet = conf.confReplaceSingleByPart(CLIENTIP, CLIENTCONF, "tfsseed", FILESIZE, String.valueOf(iSize));
		return bRet;
	}
	
	public boolean setUnitSize(int iSize)
	{
		boolean bRet = false;
		bRet = conf.confReplaceSingleByPart(CLIENTIP, CLIENTCONF, "tfsseed", UNITSIZEMIN, String.valueOf(iSize));
		if (bRet == false) return bRet;
		bRet = conf.confReplaceSingleByPart(CLIENTIP, CLIENTCONF, "tfsseed", UNITSIZEMAX, String.valueOf(iSize));
		return bRet;
	}
	
	public boolean setSeedFlag(int iFlag)
	{
		boolean bRet = false;
		bRet = conf.confReplaceSingleByPart(CLIENTIP, CLIENTCONF, "tfsseed", LOOPFLAG, String.valueOf(iFlag));
		return bRet;
	}
	
	public boolean setReadFlag(int iFlag)
	{
		boolean bRet = false;
		bRet = conf.confReplaceSingleByPart(CLIENTIP, CLIENTCONF, "tfsread", LOOPFLAG, String.valueOf(iFlag));
		return bRet;
	}	
	
	/**
	 * 
	 * @param iUnlinkRatio
	 * @return
	 */
	public boolean setUnlinkRatio(int iUnlinkRatio)
	{
		boolean bRet = false;
		bRet = conf.confReplaceSingleByPart(CLIENTIP, CLIENTCONF, "tfsseed", UNLINKRATIO, String.valueOf(iUnlinkRatio));
		return bRet;
	}

	/**
	 * 
	 * @param part
	 * @param keyWord
	 * @param value
	 * @return
	 */
	public boolean setNsConf(String part, String keyWord, String value)
	{
		boolean bRet = false;
		
		/* set NSA's conf */
		bRet = conf.confReplaceSingleByPart(NSIPA, NSCONF, part, keyWord, value);
		if (bRet == false) 
		{
			log.debug("Set " + NSIPA + " 's " + keyWord + " to " + value + " fail!");
			return bRet;
		}
		log.debug("Set " + NSIPA + keyWord + " to " + value + " success!");
		
		/* set NSB's conf */
		bRet = conf.confReplaceSingleByPart(NSIPB, NSCONF, part, keyWord, value);
		if (bRet == false) 
		{
			log.debug("Set " + NSIPB + " 's " + keyWord + " to " + value + " fail!");
			return bRet;
		}
		log.debug("Set " + NSIPA + keyWord + " to " + value + " success!");
		return bRet;
	}
	
	/**
	 * 
	 * @param grid
	 * @param bFlag
	 * @return
	 */
	public boolean setDsConf(AppGrid grid, boolean bFlag)
	{
		boolean bRet = false;
		/* Init */
		String strIp = null;
		String strConf = null;
		String strIpAddrList = null;
		String strIpAddr = grid.getCluster(0).getServer(0).getIp();
		int iServerCnt = grid.getCluster(0).getServerList().size();
		
		if (iServerCnt == 1)
		{
			strIpAddrList = grid.getCluster(0).getServer(0).getIp() + "|" + "0.0.0.0";
		} else if (iServerCnt == 2)
		{
			strIpAddrList = grid.getCluster(0).getServer(0).getIp() + "|" + grid.getCluster(0).getServer(1).getIp();
		} else {
			log.error("Nameserver's count(" + iServerCnt + ") is not correct!!!");
			return false;
		}

		if (bFlag == true)
		{
			for (int iLoop = 1; iLoop < grid.getClusterList().size(); iLoop ++)
			{
				iServerCnt = grid.getCluster(iLoop).getServerList().size();
				for (int jLoop = 0; jLoop < iServerCnt; jLoop ++)
				{
					strIp = grid.getCluster(iLoop).getServer(jLoop).getIp();
					strConf = grid.getCluster(iLoop).getServer(jLoop).getDir() + grid.getCluster(iLoop).getServer(jLoop).getConfname();
					bRet = conf.confReplaceSingle(strIp, strConf, "ip_addr", strIpAddr);
					if (bRet == false)
					{
						return bRet;
					}
					bRet = conf.confReplaceSingle(strIp, strConf, "ip_addr_list", strIpAddrList);
					if (bRet == false)
					{
						return bRet;
					}
				}
			}
			bFlag = false;
		}
		return bRet;
	}

	/**
	 * 
	 * @param strIp
	 * @return
	 */
	public boolean resetFailCount(String strIp)
	{
		boolean bRet = false;
		String macName = null;
		if (strIp == NSIPA)
		{
			macName = NSMACA;
		} else if (strIp == NSIPB)
		{
			macName = NSMACB;
		} else {
			log.error("IP address(" + strIp + ") is not a ns's ip!!!");
			return bRet;
		}
		bRet = HA.setFailCountBase(strIp, NSRES, macName, FAILCOUNTNOR);
		return bRet;
	}
	
	/**
	 * 
	 * @return
	 */
	public boolean resetAllFailCnt()
	{
		log.info("Reset failcount start ===>");
		boolean bRet = resetFailCount(NSIPA);
		if (bRet == false) return bRet;
		
		bRet = resetFailCount(NSIPB);
		log.info("Reset failcount end ===>");
		return bRet;	
	}
	
	
	
	
	/**
	 * 
	 * @return
	 */
	public boolean migrateVip()
	{
		boolean bRet = false;
		log.info("Migrate vip start ===>");
		bRet = HA.setVipMigrateBase(NSIPA, IPALIAS, NSMACA);
		log.info("Migrate vip end ===>");
		return bRet;	
	}
	
	/**
	 * 
	 * @param tarIp
	 * @param fileName
	 * @param keyWord
	 * @return
	 */
	public float getRateEnd(String tarIp, String fileName, String keyWord)
	{
		boolean bRet = false;
		float fRet = -1;
		ArrayList<String> filter = new ArrayList<String>();
		ArrayList<Float> context = new ArrayList<Float>();
		filter.add("%");
		filter.add(",");
		filter.add("rate:");
		bRet = Log.scanTailFloat(tarIp, fileName, keyWord, TAILLINE, TAILRATECOL, filter, context);
		if ((bRet == false) || (context.size() != 1))
		{
			return fRet;
		}
		fRet = context.get(0);
		return fRet;	
	}
	
	/**
	 * 
	 * @param tarIp
	 * @param fileName
	 * @param keyWord
	 * @return
	 */
	public float getTPSEnd(String tarIp, String fileName, String keyWord)
	{
		boolean bRet = false;
		float fRet = -1;
		ArrayList<String> filter = new ArrayList<String>();
		ArrayList<Float> context = new ArrayList<Float>();
		filter.add(",");
		filter.add("TPS:");
		bRet = Log.scanTailFloat(tarIp, fileName, keyWord, TAILLINE, TAILTPSCOL, filter, context);
		if ((bRet == false) || (context.size() != 1))
		{
			return fRet;
		}
		fRet = context.get(0);
		return fRet;	
	}
	
	/**
	 * 
	 * @param tarIp
	 * @param fileName
	 * @param keyWord
	 * @return
	 */
	public float getRateRun(String tarIp, String fileName, String keyWord){
		boolean bRet = false;
		float fRet = -1;
		int iFailCount = 0;
		ArrayList<String> filter = new ArrayList<String>();
		ArrayList<Float> context = new ArrayList<Float>();
		filter.add(",");
		filter.add("sum_failed:");
		bRet = Log.scanAllFloat(tarIp, fileName, keyWord, RUNRATECOL, filter, context);
		if ((bRet == false) || (context.size() <= 1))
		{
			return fRet;
		}
		iFailCount = (int) (context.get(context.size() - 1) - context.get(0));
		fRet = (context.size() * 1000 - iFailCount) / (context.size() * 10); 
		return fRet;	
	}
	
	/**
	 * 
	 * @param tarIp
	 * @param fileName
	 * @param keyWord
	 * @return
	 */
	public float getTPSRun(String tarIp, String fileName, String keyWord){
		boolean bRet = false;
		float fRet = -1;
		float fTps = 0;
		ArrayList<String> filter = new ArrayList<String>();
		ArrayList<Float> context = new ArrayList<Float>();
		filter.add(",");
		filter.add("TPS(T/s):");
		bRet = Log.scanAllFloat(tarIp, fileName, keyWord, RUNTPSCOL, filter, context);
		if ((bRet == false) || (context.size() <= 1))
		{
			return fRet;
		}
		for (int iLoop = 0; iLoop < context.size(); iLoop ++)
		{
			fTps += context.get(iLoop);
		}
		fRet = fTps / context.size();
		return fRet;	
	}
	
	public boolean setClientThread(int iThreadCount){
		boolean bRet = false;
		bRet = conf.confReplaceSingle(CLIENTIP, CLIENTCONF, "thread_count", String.valueOf(iThreadCount));
		if (bRet == false)
		{
			return bRet;
		}
		return bRet;
	}
	
	public boolean setClientMode(int iMode){
		boolean bRet = false;
		bRet = conf.confReplaceSingle(CLIENTIP, CLIENTCONF, "seedSetMode", String.valueOf(iMode));
		if (bRet == false)
		{
			return bRet;
		}
		return bRet;
	}
	
	public boolean checkRateEnd_old(float fStd, int iMode){
		boolean bRet = false;
		float fRet = 0;
		if ((iMode & WRITEONLY) != 0)
		{
			fRet = getRateEnd(CLIENTIP, tfsSeedClient.getLogs(), WRITEFILESTATIS);
			if (fRet == -1)
			{
				return bRet;
			}
			if (fRet < fStd)
			{
				log.error("Write success rate(" + fRet + "%) if lower than " + fStd + "% !!!");
				return bRet;
			} else {
				log.info("Write success rate(" + fRet + "%) if higher than " + fStd + "% !!!");
			}
		}
		if ((iMode & WRITEUNI) != 0)
		{
			fRet = getRateEnd(CLIENTIP, tfsSeedClient.getLogs(), SAVEUNIQUESTATIS);
			if (fRet == -1)
			{
				return bRet;
			}
			if (fRet < fStd)
			{
				log.error("SaveUnique success rate(" + fRet + "%) if lower than " + fStd + "% !!!");
				return bRet;
			} else {
				log.info("SaveUnique success rate(" + fRet + "%) if higher than " + fStd + "% !!!");
			}
			fRet = getRateEnd(CLIENTIP, tfsSeedClient.getLogs(), UNIQUESTATIS);
			if (fRet == -1)
			{
				return bRet;
			}
			if (fRet < fStd)
			{
				log.error("Unique success rate(" + fRet + "%) if lower than " + fStd + "% !!!");
				return bRet;
			} else {
				log.info("Unique success rate(" + fRet + "%) if higher than " + fStd + "% !!!");
			}
		}
		if ((iMode & READ) != 0)
		{			
			fRet = getRateEnd(CLIENTIP, tfsReadClient.getLogs(), READFILESTATIS);
			if (fRet == -1)
			{
				return bRet;
			}	
			if (fRet < fStd)
			{
				log.error("Read success rate(" + fRet + "%) if lower than " + fStd + "% !!!");
				return bRet;
			} else {
				log.info("Read success rate(" + fRet + "%) if higher than " + fStd + "% !!!");
			}
		}
		if ((iMode & READSLAVE) != 0)
		{			
//			fRet = getRateEnd(CLIENTIP, tfsReadClient_slave.getLogs(), READFILESTATIS);
//			if (fRet == -1)
//			{
//				return bRet;
//			}	
//			if (fRet < fStd)
//			{
//				log.error("Read success rate(" + fRet + "%) if lower than " + fStd + "% !!!");
//				return bRet;
//			} else {
//				log.info("Read_slave success rate(" + fRet + "%) if higher than " + fStd + "% !!!");
//			}
		}
		
		if ((iMode & UNLINK) != 0)
		{			
			fRet = getRateEnd(CLIENTIP, tfsReadClient.getLogs(), UNLINKSTATIS);
			if (fRet == -1)
			{
				return bRet;
			}	
			if (fRet < fStd)
			{
				log.error("Read success rate(" + fRet + "%) if lower than " + fStd + "% !!!");
				return bRet;
			} else {
				log.info("Read_slave success rate(" + fRet + "%) if higher than " + fStd + "% !!!");
			}
		}
		
		return true;
	}
	
	public boolean checkRateEnd(float fStd, int iMode){
		boolean bRet = false;
		float fRet = 0;
		if ((iMode & WRITEONLY) != 0)
		{
			fRet = getRateEnd(CLIENTIP, tfsSeedClient.getLogs() + caseName, WRITEFILESTATIS);
			if (fRet == -1)
			{
				return bRet;
			}
			if (fRet < fStd)
			{
				log.error("Write success rate(" + fRet + "%) if lower than " + fStd + "% !!!");
				return bRet;
			} else {
				log.info("Write success rate(" + fRet + "%) if higher than " + fStd + "% !!!");
			}
		}

		if ((iMode & READ) != 0)
		{			
			fRet = getRateEnd(CLIENTIP, tfsReadClient.getLogs() + caseName, READFILESTATIS);
			if (fRet == -1)
			{
				return bRet;
			}	
			if (fRet < fStd)
			{
				log.error("Read success rate(" + fRet + "%) if lower than " + fStd + "% !!!");
				return bRet;
			} else {
				log.info("Read success rate(" + fRet + "%) if higher than " + fStd + "% !!!");
			}
		}
		
		if ((iMode & UNLINK) != 0)
		{			
			fRet = getRateEnd(CLIENTIP, tfsUnlinkClient.getLogs() + caseName, UNLINKSTATIS);
			if (fRet == -1)
			{
				return bRet;
			}	
			if (fRet < fStd)
			{
				log.error("Read success rate(" + fRet + "%) if lower than " + fStd + "% !!!");
				return bRet;
			} else {
				log.info("Read_slave success rate(" + fRet + "%) if higher than " + fStd + "% !!!");
			}
		}
		
		return true;
	}
	
	public boolean checkRateRunByLog(float fStd, int iMode, String logFile){
		boolean bRet = false;
		float fRet = 0;
		if ((iMode & WRITEONLY) != 0)
		{
			fRet = getRateRun(CLIENTIP, logFile, WRITEFILE);
			if (fRet == -1)
			{
				return bRet;
			}
			if (fRet < fStd)
			{
				log.error("Write success rate(" + fRet + "%) is lower than " + fStd + "% !!!");
				return bRet;
			} else {
				log.info("Write success rate(" + fRet + "%) is higher than " + fStd + "%");
			}
		}
		if ((iMode & WRITEUNI) != 0)
		{
			fRet = getRateRun(CLIENTIP, logFile, SAVEUNIQUEFILE);
			if (fRet == -1)
			{
				return bRet;
			}
			if (fRet < fStd)
			{
				log.error("SaveUnique success rate(" + fRet + "%) is lower than " + fStd + "% !!!");
				return bRet;
			} else {
				log.info("SaveUnique success rate(" + fRet + "%) is higher than " + fStd + "%");
			}
			fRet = getRateRun(CLIENTIP, logFile, UNIQUE);
			if (fRet == -1)
			{
				return bRet;
			}
			if (fRet < fStd)
			{
				log.error("Unique success rate(" + fRet + "%) is lower than " + fStd + "% !!!");
				return bRet;
			} else {
				log.info("Unique success rate(" + fRet + "%) is higher than " + fStd + "%");
			}
		}
		if ((iMode & READ) != 0)
		{			
			fRet = getRateRun(CLIENTIP, logFile, READFILE);
			if (fRet == -1)
			{
				return bRet;
			}	
			if (fRet < fStd)
			{
				log.error("Read success rate(" + fRet + "%) is lower than " + fStd + "% !!!");
				return bRet;
			} else {
				log.info("Read success rate(" + fRet + "%) is higher than " + fStd + "%");
			}
		}
		
		if ((iMode & UNLINK) != 0)
		{			
			fRet = getRateRun(CLIENTIP, logFile, UNLINKFILE);
			if (fRet == -1)
			{
				return bRet;
			}	
			if (fRet < fStd)
			{
				log.error("Unlink success rate(" + fRet + "%) is lower than " + fStd + "% !!!");
				return bRet;
			} else {
				log.info("Unlink success rate(" + fRet + "%) is higher than " + fStd + "%");
			}
		}
		
		return true;
	}
	
	public boolean checkRateRun(float fStd, int iMode)
	{
		boolean bRet = false;
		String sorLog = tfsSeedClient.getLogs() + caseName;
		String tarLog = createClientLog(caseName, TEMP);
		String startCmd = "tail -f " + sorLog;
		bRet = Proc.proStartBack(CLIENTIP, startCmd + " > " + tarLog);
		if (bRet == false)
		{
			return bRet;
		}
		
		/* Wait */
		sleep(SCANTIME);
		
		bRet = Proc.proStopByCmd(CLIENTIP, startCmd);
		if (bRet == false)
		{
			return bRet;
		}
		
		/* check the result */
		bRet = checkRateRunByLog( fStd,  iMode,  tarLog);
		if (bRet == false)
		{
			return bRet;
		}
		
		return bRet;
	}
	
	public boolean checkWrittenFile(float fStd){
		boolean bRet = false;
		int iRet = -1;
		
		/* Set the thread on client */
		bRet = setClientThread(READTHREAD);
		if (bRet == false)
		{
			return bRet;
		}
		
		/* Start the client process */
		bRet = tfsReadClient.start();
		if (bRet == false)
		{
			return bRet;
		}
		
		/* Monitor the client process */
		for ( ; ; )
		{
			iRet = Proc.proMonitorBase(CLIENTIP, tfsReadClient.getStartcmd());
			if (iRet <= 0)
			{
				if (iRet < 0) bRet = false;
				break;
			}
			sleep(60);
		}
		
		if (bRet == false) return bRet;
		/* Check the Result */
		bRet = checkRateEnd(fStd, READ);		
		return bRet;
	} 
	
	public boolean mvSeedFile()
	{
		boolean bRet = false;
		String seedFile = TEST_HOME + "/tfsseed_file_list.txt";
		bRet = File.fileCopy(CLIENTIP, seedFile, seedFile + caseName);
		if (bRet == false) return bRet;
		bRet = File.fileDel(CLIENTIP, seedFile);
		return bRet;
	}
	
	public boolean mvLogFile(String logName, String suffix)
	{
		boolean bRet = false;
		/* Check the log file is exist or not */
		bRet = File.checkFileBase(CLIENTIP, logName);
		if (bRet == true)
		{
			bRet = File.fileCopy(CLIENTIP, logName, logName + "." + suffix);
			if (bRet == false)
			{
				return bRet;
			}
			bRet = File.fileDel(CLIENTIP, logName);
			if (bRet == false) return bRet;
		} else {
			bRet = true;
		}
		return bRet;
	}
	
	public boolean mvLog(String suffix)
	{
		boolean bRet = false;
		bRet = mvLogFile(tfsSeedClient.getLogs(), suffix);
		if (bRet == false) return bRet;
		
		bRet = mvLogFile(tfsReadClient.getLogs(), suffix);
		if (bRet == false) return bRet;
		
//		bRet = mvLogFile(tfsReadClient_slave.getLogs(), suffix);
//		if (bRet == false) return bRet;
		
		return bRet;
	}
	
	public String createClientLog(String suffix, int iMode)
	{
		if (((iMode & WRITEONLY) != 0) || ((iMode & WRITEUNI) != 0))
		{
			return (tfsSeedClient.getLogs()+ "." + suffix);
		} else if ((iMode & READ) != 0){
			return (tfsReadClient.getLogs()+ "." + suffix);
		} else if ((iMode & TEMP) != 0){
			return (tfsSeedClient.getLogs() + ".temp." + suffix);
		//} else if ((iMode & READSLAVE) != 0){
//			return (tfsReadClient_slave.getLogs() + ".slave." + suffix);
		} else {
			log.error("Mode(" + iMode + ") error");
			return ("");
		}
	}
	
	public boolean killMasterNs()
	{
		boolean bRet = false;
		String tempIp = "";
		String tempMaster = "";
		List<AppServer> listNs = tfsGrid.getCluster(NSINDEX).getServerList();

		/* Find the master ns */
		if (listNs.get(0).getIp().equals(masterIp))
		{
			bRet = listNs.get(0).stop(KillTypeEnum.NORMALKILL, WAITTIME);
			if (bRet == false) return bRet;
			tempIp = listNs.get(0).getIp();
			tempMaster = listNs.get(1).getIp();
		} else {
			bRet = listNs.get(1).stop(KillTypeEnum.NORMALKILL, WAITTIME);
			if (bRet == false) return bRet;
			tempIp = listNs.get(1).getIp();
			tempMaster = listNs.get(0).getIp();
		}
		
		/* Wait for vip migrate */
		sleep (5);
		
		/* Check vip */
		bRet = HA.chkVipBase(tempIp);
		if (bRet == true)
		{
			log.error("VIP is not migrate yet!!!");
			return false;
		}
		
		/* Reset the failcount */
		bRet = resetFailCount(tempIp);
		if (bRet == false) return bRet;
		
		/* Set the new vip */
		masterIp = tempMaster;
		
		return bRet;
	}
	
	public boolean killSlaveNs()
	{
		boolean bRet = false;
		List<AppServer> listNs = tfsGrid.getCluster(NSINDEX).getServerList();

		/* Find the master ns */
		if (listNs.get(0).getIp().equals(masterIp))
		{
			bRet = listNs.get(0).stop(KillTypeEnum.NORMALKILL, WAITTIME);
			if (bRet == false) return bRet;
		} else {
			bRet = listNs.get(1).stop(KillTypeEnum.NORMALKILL, WAITTIME);
			if (bRet == false) return bRet;
		}
		
		/* Wait for vip migrate */
		sleep (5);
		
		/* Check vip */
		bRet = HA.chkVipBase(masterIp);
		if (bRet == true)
		{
			log.error("VIP is not on master ns yet!!!");
			return false;
		}
		
		return bRet;
	}
	
	public boolean chkBlockCnt(int times, int shouldNotExistBlockCnt)
	{
		boolean bRet = false;
		ArrayList<String> listOut = new ArrayList<String>();
		String cmd = TFS_BIN_HOME + "/ssm -s " + NSVIP + ":" + NSPORT +
			" -i block | grep \\\"" + shouldNotExistBlockCnt + "$\\\" | wc -l";
		
		for (int iLoop = 0; iLoop < times; iLoop ++)
		{
			bRet = Proc.cmdOutBase(NSIPA, cmd, null, 1, null, listOut);
			if (bRet == false) return bRet;
			
			try{
				int stillExistNum = Integer.valueOf(listOut.get(listOut.size() - 1));			
				if (stillExistNum == 0)
				{
					bRet = true;
					break;
				}
				
//				for(int jLoop = 0;jLoop < listOut.size(); jLoop ++)
//				{
//					log.info("Result: " + listOut.get(jLoop));
//				}
//				break;
			} catch (Exception e){
				e.printStackTrace();
				bRet = false;
				break;
			}
			sleep(10);
		}
		return bRet;
	}
	
	public boolean chkBlockCntBothNormal(int iBlockCnt)
	{
		boolean bRet = false;
		for (int iLoop = 0; iLoop < 4; iLoop ++)
		{
			if (iBlockCnt != iLoop)
			{
				bRet = chkBlockCntBoth(BLOCK_CHK_TIME, iLoop);
				if (bRet == false) break;
			}
		}
		
		return bRet;
	}
	
	public boolean chkBlockCntBoth(int iTimes, int shouldNotExistBlockCnt)
	{
		boolean bRet = false;
		ArrayList<String> listOut = new ArrayList<String>();
		int iLoop = 0;
		String cmd = "";
		
		for (iLoop = 0; iLoop < iTimes; iLoop ++)
		{
			cmd = TFS_BIN_HOME + "/ssm -s " + NSIPA + ":" + NSPORT +
				" -i block | grep \\\"" + shouldNotExistBlockCnt + "$\\\" | wc -l";
			bRet = Proc.cmdOutBase(NSIPA, cmd, null, 1, null, listOut);
			if (bRet == false) return bRet;
			
			try{
				int stillExistNum = Integer.valueOf(listOut.get(listOut.size() - 1));			
				if (stillExistNum == 0)
				{
					bRet = true;
					break;
				}
			} catch (Exception e){
				e.printStackTrace();
				bRet = false;
				break;
			}
			sleep(1);
		}
		
		for (int jLoop = 0; jLoop < iTimes - iLoop + 1; jLoop ++)
		{
			cmd = TFS_BIN_HOME + "/ssm -s " + NSIPB + ":" + NSPORT +
				" -i block | grep \\\"" + shouldNotExistBlockCnt + "$\\\" | wc -l";
			bRet = Proc.cmdOutBase(NSIPB, cmd, null, 1, null, listOut);
			if (bRet == false) return bRet;
			
			try{
				int stillExistNum = Integer.valueOf(listOut.get(listOut.size() - 1));			
				if (stillExistNum == 0)
				{
					bRet = true;
					break;
				}
			} catch (Exception e){
				e.printStackTrace();
				bRet = false;
				break;
			}
			sleep(1);
		}
		return bRet;
	}
	
	/**
	 * @author mingyan
	 * @param tarIp
	 * @return
	 */
	public boolean genDumpPlanLog(String tarIp)
	{
		boolean bRet = false;
		
		/* Get nameserver's log list */
		String strCmd = "ls " + TFS_LOG_HOME + "" ; 
		ArrayList<String> logNameList = new ArrayList<String>();
		bRet = Proc.cmdOutBase(tarIp, strCmd, CURR_LOG_NAME, 0, null, logNameList);
		if (bRet == false) return bRet;
		
		/* Resort nameserver's log list */
		if (logNameList.isEmpty())
		{
			log.error("Error! There is no NS log!");
			bRet = false;
			return bRet;
		}
		for (int iLoop = 0; iLoop < logNameList.size(); iLoop++)
		{
			log.debug("logNameList: " + logNameList.get(iLoop));
		}
		if (!logNameList.get(0).equals(CURR_LOG_NAME))
		{
			log.error("Error! Wrong log list!");
			bRet = false;
			return bRet;
		}
		String strCurrLog = logNameList.remove(0);
		logNameList.add(strCurrLog);
		
		for (int iLoop = 0; iLoop < logNameList.size(); iLoop++)
		{
			String ProcessLog = logNameList.get(iLoop);
			log.debug("logName: " + ProcessLog);
			if (iLoop == 0)
			{
				strCmd = "cd " + TFS_LOG_HOME + "; grep \\\"" + 
					PLANSEQNO + "\\\" " + ProcessLog + " | sed \\\"s/].*plan seqno/]plan seqno/\\\" >" + DP_LOG_NAME;
			}
			else
			{
				strCmd = "cd " + TFS_LOG_HOME + "; grep \\\"" + 
					PLANSEQNO + "\\\" " + ProcessLog + " | sed \\\"s/].*plan seqno/]plan seqno/\\\" >>" + DP_LOG_NAME;
			}
			bRet = Proc.proStartBase(tarIp, strCmd);
			if (bRet == false) return bRet;
		}
		
		return bRet;
	}

	/**
	 * @author mingyan
	 * @param tarIp
	 * @param blockList 
	 * @return
	 */
	public boolean getReplicatedBlock(String tarIp, ArrayList<String> blockList)
	{
		boolean bRet = false;
		if (blockList == null)
			return bRet;
 
		ArrayList<String> keyWords = new ArrayList<String>(); 
		ArrayList<String> filterList = new ArrayList<String>();
		keyWords.add(REPLICATE);
		keyWords.add(STATUS_FINISH);
		keyWords.add(NORMAL);
		filterList.add(",");
		bRet = Log.scanAll(tarIp, DP_LOG_NAME, keyWords, BLOCKID_DP_COL, filterList, blockList);
		if (bRet == false) return bRet;

		for (int iLoop = 0; iLoop < blockList.size(); iLoop++)
		{
			log.debug("blockList: " + blockList.get(iLoop));
		}	
		return bRet;
	}

	/**
	 * @author mingyan
	 * @param tarIp
	 * @param blockList 
	 * @return
	 */
	public boolean getMovedBlock(String tarIp, ArrayList<String> blockList)
	{
		boolean bRet = false;

		if (blockList == null)
			return bRet;
  
		ArrayList<String> keyWords = new ArrayList<String>(); 
		ArrayList<String> filterList = new ArrayList<String>();
		keyWords.add(MOVE);
		keyWords.add(STATUS_FINISH);
		keyWords.add(NORMAL);
		filterList.add(",");
		bRet = Log.scanAll(tarIp, DP_LOG_NAME, keyWords, BLOCKID_DP_COL, filterList, blockList);
		if (bRet == false) return bRet;

		for (int iLoop = 0; iLoop < blockList.size(); iLoop++)
		{
			log.debug("blockList: " + blockList.get(iLoop));
		}	
		return bRet;
	}

	/**
	 * @author mingyan
	 * @param tarIp
	 * @param blockList 
	 * @return
	 */
	public boolean getCompactedBlock(String tarIp, ArrayList<String> blockList)
	{
		boolean bRet = false;

		if (blockList == null)
			return bRet;
   
		ArrayList<String> keyWords = new ArrayList<String>(); 
		ArrayList<String> filterList = new ArrayList<String>();
		keyWords.add(COMPACT);
		keyWords.add(STATUS_FINISH);
		keyWords.add(NORMAL);
		filterList.add(",");
		bRet = Log.scanAll(tarIp, DP_LOG_NAME, keyWords, BLOCKID_DP_COL, filterList, blockList);
		if (bRet == false) return bRet;

		for (int iLoop = 0; iLoop < blockList.size(); iLoop++)
		{
			log.debug("blockList: " + blockList.get(iLoop));
		}	
		return bRet;
	}

	/**
	 * @author mingyan
	 * @param tarIp
	 * @param blockList 
	 * @return
	 */
	public boolean getDeletedBlock(String tarIp, ArrayList<String> blockList)
	{
		boolean bRet = false;
    
		if (blockList == null)
			return bRet;
 
		ArrayList<String> keyWords = new ArrayList<String>(); 
		ArrayList<String> filterList = new ArrayList<String>();
		keyWords.add(DELETE);
		keyWords.add(STATUS_FINISH);
		keyWords.add(NORMAL);
		filterList.add(",");
		bRet = Log.scanAll(tarIp, DP_LOG_NAME, keyWords, BLOCKID_DP_COL, filterList, blockList);
		if (bRet == false) return bRet;

		for (int iLoop = 0; iLoop < blockList.size(); iLoop++)
		{
			log.debug("blockList: " + blockList.get(iLoop));
		}	
		return bRet;
	}

	/**
	 * @author mingyan
	 * @param tarIp
	 * @param blockList 
	 * @return
	 */
	public boolean getEmergencyReplicatedBlock(String tarIp, ArrayList<String> blockList)
	{
		boolean bRet = false;

		if (blockList == null)
			return bRet;
   
		ArrayList<String> keyWords = new ArrayList<String>(); 
		ArrayList<String> filterList = new ArrayList<String>();
		keyWords.add(REPLICATE);
		keyWords.add(STATUS_FINISH);
		keyWords.add(EMERGENCY);
		filterList.add(",");
		bRet = Log.scanAll(tarIp, DP_LOG_NAME, keyWords, BLOCKID_DP_COL, filterList, blockList);
		if (bRet == false) return bRet;

		for (int iLoop = 0; iLoop < blockList.size(); iLoop++)
		{
			log.debug("blockList: " + blockList.get(iLoop));
		}	
		return bRet;
	}

	/**
	 * @author mingyan
	 * @param tarIp
	 * @param planNum 
	 * @param planPriority
	 * @return
	 */
	public boolean getPreviousXPlanPriority(String tarIp, int planNum, int checkCountLimit, ArrayList<String> planPriority)
	{
		boolean bRet = false;

		if (planPriority == null)
			return bRet;
		
		/* Get the previous planNum plans' priority */
		String strCmd = "grep -m "+ planNum + " \\\"" + PLANSEQNO + "\\\" " + 
			NS_LOG_NAME + " | grep " + "\\\"" + STATUS_BEGIN + "\\\"" + 
			" | sed \\\"s/].*plan seqno/]plan seqno/\\\" | awk '{print $" +
			PRIORITY_DP_COL + "}'";

		int checkCount = 0;
		while (checkCount < checkCountLimit)
		{
			bRet = Proc.proStartBase(tarIp, strCmd, planPriority);
			if (planPriority.size() > 0)
				break;
			if (bRet == false) 
				return bRet;		
			checkCount++;
		}

		return bRet;
	}

	/**
	 * @author mingyan
	 * @param tarIp
	 * @param planNum 
	 * @return
	 */
	public boolean chkPreviousPlanIsEmergency(int checkNum, int checkCountLimit)
	{
		boolean bRet = false;
		
		/* Get the previous checkNum plans' priority */
		ArrayList<String> output = new ArrayList<String>();
		bRet = getPreviousXPlanPriority(NSVIP, checkNum, checkCountLimit, output);
		if (bRet == false) return bRet;
		
		int count = 0;
		for (int iLoop = 0; iLoop < output.size(); iLoop++)
		{
			log.debug("planPrios: " + output.get(iLoop) + ", count:" + count);
			if (output.get(iLoop).equals("emergency"))
				count++;
			else
				break;
		}

		if (count > 0)
			bRet = true;

		return bRet;
	}
	
	/**
	 * @author mingyan 
	 * @param planType 
	 * @param checkCountLimit
	 * @return
	 */
	public boolean checkPlan(PlanType planType, int checkCountLimit)
	{
		boolean bRet = false;
		
		ArrayList<String> blockList = new ArrayList<String>();
		int checkCount = 0;
		while (checkCount < checkCountLimit)
		{
			bRet = genDumpPlanLog(NSVIP);		
			switch (planType)
			{
				case PLAN_TYPE_REPLICATE:
					bRet = getReplicatedBlock(NSVIP, blockList);
					break;
				case PLAN_TYPE_EMERG_REPLICATE:
					bRet = getEmergencyReplicatedBlock(NSVIP, blockList);
					break;
				case PLAN_TYPE_MOVE:
					bRet = getMovedBlock(NSVIP, blockList);
					break;				
				case PLAN_TYPE_COMPACT:
					bRet = getCompactedBlock(NSVIP, blockList);
					break;				
				case PLAN_TYPE_DELETE:
					bRet = getDeletedBlock(NSVIP, blockList);
					break;			
			}
			checkCount++;
			if (bRet && (blockList.size() > 0))
				break;
			sleep(1);
		}
		
		return bRet;
	}
	
	/**
	 * @author mingyan
	 * @return
	 */
	public boolean checkInterrupt(int count)
	{
		boolean bRet = false;
		
		String strCmd = "grep \"receive interrupt: 1, pending plan list size:\" | wc -l" + NS_LOG_NAME; 
		ArrayList<String> output = new ArrayList<String>();
		bRet = Proc.proStartBase(NSVIP, strCmd, output);
		if (bRet == false) return bRet;
		int rtn = 0;
		if (output.size() > 0)
			rtn = Integer.parseInt(output.get(0));
		if (rtn >= count)
			bRet = true;
		
		return bRet;
	}
	
	/**
	 * @author mingyan
	 * @return
	 */
	public boolean rotateLog()
	{
		boolean bRet = false;
   
		/* Use admintool to rotate ns log*/
		String strCmd = TFS_BIN_HOME + "/admintool -n -s " + NSVIP + ":" + NSPORT + " -i 'rotatelog " + NSVIP + ":" + NSPORT + "'";     
		bRet = Proc.proStartBase(NSIPA, strCmd);
		if (bRet == false) return bRet;
		
		return bRet;
	}
	
	
	/**
	 * @author mingyan 
	 * @param tarIp
	 * @param blockList 
	 * @param respectCopyNum
	 * @return
	 */
	public boolean checkBlockCopyNum(ArrayList<String> blockList, int respectCopyNum)
	{
		boolean bRet = false;
    
		if (blockList == null || blockList.size() == 0)
		{
			log.error("Error! blockList is null or empty!");
			return bRet;
		}
    
		/* Use ssm tool to get blockList*/
		String strCmd = TFS_BIN_HOME + "/ssm -s " + NSVIP + ":" + NSPORT + " -i block > " + BL_LOG_NAME;     
		bRet = Proc.proStartBase(NSIPA, strCmd);
		if (bRet == false) return bRet;
		
		/* Check copy num */
		ArrayList<String> copyNumList = new ArrayList<String>();
		int correct = 0;
		bRet = Log.scanAll(NSIPA, BL_LOG_NAME, blockList, BLOCKID_BL_COL, COPYNUM_BL_COL, copyNumList);
		for (int iLoop = 0; iLoop < copyNumList.size(); iLoop++)
		{
			//log.debug("copyNum: " + copyNumList.get(iLoop));
			if (respectCopyNum == Integer.parseInt(copyNumList.get(iLoop)))
				correct++;
		}
		log.debug("respectCopyNum: " + respectCopyNum + ", correct block num: " + correct);

		if (bRet == false) return bRet;
    
		return bRet;
	} 
	
	/**
	 * @author mingyan 
	 * @param tarIp
	 * @param blockList 
	 * @param respectCopyNum
	 * @return
	 */
	public boolean checkBlockDelFileCnt(ArrayList<String> blockList, int respectDelFileCnt)
	{
		boolean bRet = false;
    
		if (blockList == null || blockList.size() == 0)
		{
			log.error("Error! blockList is null or empty!");
			return bRet;
		}
    
		/* Use ssm tool to get blockList*/
		String strCmd = TFS_BIN_HOME + "/ssm -s " + NSVIP + ":" + NSPORT + " -i block > " + BL_LOG_NAME;     
		bRet = Proc.proStartBase(NSIPA, strCmd);
		if (bRet == false) return bRet;
		
		/* Check copy num */
		ArrayList<String> delFileCntList = new ArrayList<String>();
		int correct = 0;
		bRet = Log.scanAll(NSIPA, BL_LOG_NAME, blockList, BLOCKID_BL_COL, DELFILE_BL_COL, delFileCntList);
		for (int iLoop = 0; iLoop < delFileCntList.size(); iLoop++)
		{
			//log.debug("copyNum: " + copyNumList.get(iLoop));
			if (respectDelFileCnt == Integer.parseInt(delFileCntList.get(iLoop)))
				correct++;
		}
		log.debug("respectDelFileCnt: " + respectDelFileCnt + ", correct block num: " + correct);

		if (bRet == false) return bRet;
    
		return bRet;
	} 
	
	/**
	 * @author mingyan 
	 * @param tarIp
	 * @param blockList 
	 * @return
	 */
	public boolean getCompactableBlock(ArrayList<String> blockList)
	{
		boolean bRet = false;
    
		if (blockList == null)
		{
			log.error("Error! blockList is null!");
			return bRet;
		}
    
		/* Use ssm tool to get blockList*/
		String strCmd = TFS_BIN_HOME + "/ssm -s " + NSVIP + ":" + NSPORT + " -i block > " + BL_LOG_NAME;     
		bRet = Proc.proStartBase(NSIPA, strCmd);
		if (bRet == false) return bRet;
		
		/* Find the blocks whose del_file count is not 0 */
		bRet = Log.scanAll(NSIPA, BL_LOG_NAME, " > 0", DELFILE_BL_COL, BLOCKID_BL_COL, blockList);

		/* The 1st one would be "BLOCK_ID" remove it */
		blockList.remove(0);
		
		if (bRet == false) return bRet;
    
		return bRet;
	}
	
	/**
	 * @author mingyan 
	 * @param tarIp
	 * @param blockList 
	 * @return
	 */
	public boolean compactBlock()
	{
		boolean bRet = false;
		
		/* Get compactable block */
		ArrayList<String> blockList = new ArrayList<String>();
		
		/* Use ssm tool to get blockList*/
		String strCmd = TFS_BIN_HOME + "/ssm -s " + NSVIP + ":" + NSPORT + " -i block > " + BL_LOG_NAME;     
		bRet = Proc.proStartBase(NSIPA, strCmd);
		if (bRet == false) return bRet;
		
		/* Find the blocks whose del_file count is not 0 */
		bRet = Log.scanAll(NSIPA, BL_LOG_NAME, " > 0", DELFILE_BL_COL, BLOCKID_BL_COL, blockList);

		/* The 1st one would be "BLOCK_ID" remove it */
		blockList.remove(0);
		
		/* Use admintool to compact*/
		ArrayList<String> result = new ArrayList<String>();
		for (int iLoop = 0; iLoop < blockList.size(); iLoop++)
		{
			strCmd = TFS_BIN_HOME + "/admintool -n -s " + NSVIP + ":" + 
				NSPORT + " -i 'compactblk " + blockList.get(iLoop) + "'";
			bRet = Proc.proStartBase(NSIPA, strCmd, result);
			if (bRet == false) 
			{
				blockList.remove(iLoop);
				continue;
			}
			if (result.get(0).indexOf("success")== -1)
				bRet = false; 
			if (bRet == false) 
			{
				blockList.remove(iLoop);
				continue;	
			}
		}
		
		if (bRet == false) return bRet;

		/* Check Compact result */
		bRet = checkBlockDelFileCnt(blockList, 0);
		
		return bRet;
	} 

	/**
	 * @author mingyan 
	 * @param value
	 * @return
	 */
	public boolean setMaxReplication(int value)
	{
		boolean bRet = false;
    
		/* Use admintool to set */
		String strCmd = TFS_BIN_HOME + "/admintool -n -s " + NSVIP + 
			":" + NSPORT + " -i 'param max_replication set " + value + "'";  
		ArrayList<String> result = new ArrayList<String>();
		bRet = Proc.proStartBase(NSIPA, strCmd, result);
		if (bRet == false) return bRet;
		
		if (result.get(0).indexOf("success")!= -1)
			bRet = true;
		else
			bRet = false;
    
		return bRet;
	}
	
	/**
	 * @author mingyan 
	 * @param value
	 * @return
	 */
	public boolean setMinReplication(int value)
	{
		boolean bRet = false;
    
		/* Use admintool to set */
		String strCmd = TFS_BIN_HOME + "/admintool -n -s " + NSVIP + 
			":" + NSPORT + " -i 'param min_replication set " + value + "'";     
		ArrayList<String> result = new ArrayList<String>();
		bRet = Proc.proStartBase(NSIPA, strCmd, result);		
		if (bRet == false) return bRet;
		
		if (result.get(0).indexOf("success")!= -1)
			bRet = true;
		else
			bRet = false;   
		return bRet;
	}
	
	
	/**
	 * @author mingyan 
	 * @param value
	 * @return
	 */
	public boolean setBalanceMaxDiffBlockNum(int value)
	{
		boolean bRet = false;
    
		/* Use admintool to set */
		String strCmd = TFS_BIN_HOME + "/admintool -n -s " + NSVIP + 
			":" + NSPORT + " -i 'param balance_max_diff_block_num set " + value + "'";     
		ArrayList<String> result = new ArrayList<String>();
		bRet = Proc.proStartBase(NSIPA, strCmd, result);
		if (bRet == false) return bRet;
		
		if (result.get(0).indexOf("success")!= -1)
			bRet = true;
		else
			bRet = false;   
		return bRet;
	}

	/**
	 * @author mingyan 
	 * @param value
	 * @return
	 */
	public boolean setObjectDeadMaxTime(int value)
	{
		boolean bRet = false;
    
		/* Use admintool to set */
		String strCmd = TFS_BIN_HOME + "/admintool -n -s " + NSVIP + 
			":" + NSPORT + " -i 'param object_dead_max_time set " + value + "'";     
		ArrayList<String> result = new ArrayList<String>();
		bRet = Proc.proStartBase(NSIPA, strCmd, result);
		if (bRet == false) return bRet;
		
		if (result.get(0).indexOf("success")!= -1)
			bRet = true;
		else
			bRet = false;   
		return bRet;
	}
	
	/**
	 * @author mingyan 
	 * @param blockDis
	 * @return
	 */
	public boolean getBlockDistribution(HashMap<String, Integer> blockDis)
	{
		boolean bRet = false;
    
		if (blockDis == null)
		{
			log.error("Error! blockDis is null!");
			return bRet;
		}
		
		/* Use ssm tool to get it */
		String strCmd = TFS_BIN_HOME + "/ssm -s " + NSVIP + 
			":" + NSPORT + " -i server";
		ArrayList<String> dsIpList = new ArrayList<String>();
		bRet = Proc.cmdOutBase(NSIPA, strCmd, "^ *[1-9]", DSIP_SL_COL, null, dsIpList);
		if (bRet == false) return bRet;
		
		ArrayList<String> blkCntList = new ArrayList<String>();
		bRet = Proc.cmdOutBase(NSIPA, strCmd, "^ *[1-9]", BLKCNT_SL_COL, null, blkCntList);
		if (bRet == false) return bRet;
		
		if (dsIpList.size() != blkCntList.size())
		{
			bRet = false;
			log.error("dsIpList's size not eqal blkCntList's size!");
			return bRet;
		}
		for (int iLoop = 0; iLoop < dsIpList.size(); iLoop++)
		{
			log.debug("ds: " + dsIpList.get(iLoop) + ", blkCnt: " + blkCntList.get(iLoop));
			blockDis.put(dsIpList.get(iLoop), Integer.parseInt(blkCntList.get(iLoop)));
		}
    
		return bRet;
	}
	
	/**
	 * @author mingyan 
	 * @param blockDisBefore
	 * @param blockDisAfter
	 * @return
	 */
	public boolean checkWriteBalanceStatus(HashMap<String, Integer> blockDisBefore, HashMap<String, Integer> blockDisAfter)
	{
		boolean bRet = false;
    
		if (blockDisBefore == null || blockDisBefore.isEmpty() || blockDisAfter == null || blockDisAfter.isEmpty() )
		{
			log.error("Error! blockDis is null or empty!");
			return bRet;
		}
		
		/* Get new added block count on each ds */
		ArrayList<Integer> diffBlkCntList = new ArrayList<Integer>();
		Iterator<Entry<String, Integer>>  iter = blockDisAfter.entrySet().iterator();
		String dsIp;
		java.util.Map.Entry<String, Integer> entry;
		while(iter.hasNext())
		{
			entry = iter.next();
			dsIp = entry.getKey();
			diffBlkCntList.add(entry.getValue() - blockDisBefore.get(dsIp));
			log.debug("ds: " + dsIp + " blk num before: " + blockDisBefore.get(dsIp) + 
					", after: " + entry.getValue());
		}
		
		/* Compute the avg num of the added blk cnt */		
		int sum = 0;
		int avg = 0;
		for (int iLoop = 0; iLoop < diffBlkCntList.size(); iLoop++)
		{
			log.debug(diffBlkCntList.get(iLoop));
			sum += diffBlkCntList.get(iLoop);
		}
		avg = sum / diffBlkCntList.size();
		log.debug("blk num diff sum: " + sum + ", avg: " + avg);
		
		/* If some one is far from avg, fail*/
		for (int iLoop = 0; iLoop < diffBlkCntList.size(); iLoop++)
		{
			if (diffBlkCntList.get(iLoop) - avg > BLK_DIFF_THRESHOLD)
			{
				bRet = false;
				return bRet;
			}
		}
		bRet = true;
		return bRet;
	}
	
	/**
	 * @author mingyan 
	 * @param interval
	 * @param count
	 * @return
	 */
	public boolean networkTrafMonStart(int interval, int count)
	{	
		boolean bRet = false;
		Date date = new Date();
		SAR_DSA_LOG += date.toString() + ".log";
		String strCmd = "sar -n DEV -o " + SAR_DSA_LOG + " " + interval + " " + count;
		log.info("Sar command start ===>");
		bRet = Proc.proStartBackroundBase(DSIPA, strCmd);
		if (bRet == false) return bRet;

		date = new Date();
		SAR_DSB_LOG += date.toString() + ".log";
		strCmd = "sar -n DEV -o " + SAR_DSB_LOG + " " + interval + " " + count;
		bRet = Proc.proStartBackroundBase(DSIPB, strCmd);		
		log.info("Sar command end ===>");		

		return bRet;		
	}
	
	/**
	 * @author mingyan 
	 * @param ethIndex
	 * @param col
	 * @return
	 */
	public boolean chkNetworkTrafBalance(String ethIndex, int col)
	{	
		boolean bRet = false;
		
		String strCmd = "sar -n DEV -f " + SAR_DSA_LOG;
		ArrayList<String> keyWords = new ArrayList<String>();
		ArrayList<String> output = new ArrayList<String>();
		keyWords.add("Average");
		keyWords.add(ethIndex);
		bRet = Proc.cmdOutBase2(DSIPA, strCmd, keyWords, col, null, output);
		if (bRet == false) return bRet;
		
		if (output.size() == 0)
			return false;
		double rtxbytPerSecDSA = Double.parseDouble(output.get(0));	
		if (col == RXBYTPERSEC_SD_COL)
			log.debug("rxbytPerSecDSA: " + output.get(0));
		else if (col == TXBYTPERSEC_SD_COL)
			log.debug("txbytPerSecDSA: " + output.get(0));
		
		strCmd = "sar -n DEV -f " + SAR_DSB_LOG;
		output.clear();
		bRet = Proc.cmdOutBase2(DSIPB, strCmd, keyWords, col, null, output);
		if (bRet == false) return bRet;		
		
		if (output.size() == 0)
			return false;
		double rtxbytPerSecDSB = Double.parseDouble(output.get(0));	
		if (col == RXBYTPERSEC_SD_COL)
			log.debug("rxbytPerSecDSB: " + output.get(0));
		else if (col == TXBYTPERSEC_SD_COL)
			log.debug("txbytPerSecDSB: " + output.get(0));
		
		double temp;
		if (rtxbytPerSecDSA < rtxbytPerSecDSB)
		{
			temp = rtxbytPerSecDSA;
			rtxbytPerSecDSA = rtxbytPerSecDSB;
			rtxbytPerSecDSB = temp;
		}
		
		double diff = rtxbytPerSecDSA - rtxbytPerSecDSB;
		if (col == RXBYTPERSEC_SD_COL)
			log.debug("diff of rxbytPerSecDs: " + diff);
		else if (col == TXBYTPERSEC_SD_COL)
			log.debug("diff of txbytPerSecDs: " + diff);
		
		if (diff > NETWORKTRAF_IMBALANCE_THRESHOLD)
			bRet = false;
		
		return bRet;
	}
	
	/**
	 * @author mingyan 
	 * @param usedCap
	 * @return
	 */
	public boolean getUsedCap(HashMap<String, Double> usedCap)
	{
		boolean bRet = false;
    
		/* Use ssm to query */
		String strCmd = TFS_BIN_HOME + "/ssm -s " + NSVIP + 
			":" + NSPORT + " -i server";
		ArrayList<String> result = new ArrayList<String>();
		bRet = Proc.cmdOutBase(NSIPA, strCmd, "TOTAL:", USED_CAP_COL, null, result);
		if (bRet == false) return bRet;
		
		if (result.size() != 1) return false;
		String strValue = result.get(0);
		double transFactor = 1.0;
		if (strValue.endsWith("M"))
		{
			transFactor = 1024;
		}
		else if (strValue.endsWith("T"))
		{
			transFactor = 1/1024;
		}
		strValue = strValue.substring(0, strValue.length()-2);
		double valueBefore = Double.parseDouble(strValue);
		valueBefore /= transFactor;
		log.debug("usedCapBefore: " + valueBefore);
		usedCap.put("Before", valueBefore);
		
		bRet = true;
    
		return bRet;
	}	

	/**
	 * @author mingyan 
	 * @param value
	 * @return
	 */
	public boolean chkUsedCap(double chkValue)
	{
		boolean bRet = false;
    
		/* Use ssm to query */
		String strCmd = TFS_BIN_HOME + "/ssm -s " + NSVIP + 
			":" + NSPORT + " -i server";
		ArrayList<String> result = new ArrayList<String>();
		double value;
		do {
			bRet = Proc.cmdOutBase(NSIPA, strCmd, "TOTAL:", USED_CAP_COL, null, result);
			if (bRet == false) return bRet;
			
			if (result.size() != 1) return false;
			String strValue = result.get(0);
			double transFactor = 1.0;
			if (strValue.endsWith("M"))
			{
				transFactor = 1024;
			}
			else if (strValue.endsWith("T"))
			{
				transFactor = 1/1024;
			}
			strValue = strValue.substring(0, strValue.length()-2);
			value = Double.parseDouble(strValue);
			value /= transFactor;
			log.debug("usedCapNow: " + value);
			result.clear();
			sleep(10);
		} while(value < chkValue);
		
		bRet = true;
    
		return bRet;
	}
	
	/**
	 * @author mingyan 
	 * @param blkCnt
	 * @return
	 */
	public boolean getCurrBlkCnt(HashMap<String, Integer> blkCnt)
	{
		boolean bRet = false;
    
		/* Use ssm to query */
		String strCmd = TFS_BIN_HOME + "/ssm -s " + NSVIP + 
			":" + NSPORT + " -i server";
		ArrayList<String> result = new ArrayList<String>();
		String strValue = "";
		String strPreValue;
		do {
			bRet = Proc.cmdOutBase(NSIPA, strCmd, "TOTAL:", CURR_BLKCNT_COL, null, result);
			if (bRet == false) return bRet;
			
			if (result.size() != 1) return false;
			strPreValue = strValue;
			strValue = result.get(0);
			result.clear();
			sleep(20);
		} while (strValue.equals("0") || !strPreValue.equals(strValue));
		int valueBefore = Integer.parseInt(strValue);
		log.debug("blkCntBefore: " + valueBefore);
		blkCnt.put("Before", valueBefore);
		
		bRet = true;
    
		return bRet;
	}	

	/**
	 * @author mingyan 
	 * @param chkValue
	 * @return
	 */
	public boolean chkCurrBlkCnt(int chkValue)
	{
		boolean bRet = false;
    
		/* Use ssm to query */
		String strCmd = TFS_BIN_HOME + "/ssm -s " + NSVIP + 
			":" + NSPORT + " -i server";
		ArrayList<String> result = new ArrayList<String>();
		double value;
		do {
			bRet = Proc.cmdOutBase(NSIPA, strCmd, "TOTAL:", CURR_BLKCNT_COL, null, result);
			if (bRet == false) return bRet;
			
			if (result.size() != 1) return false;
			String strValue = result.get(0);
			value = Integer.parseInt(strValue);
			log.debug("currBlkCnt: " + value);
			result.clear();
			sleep(20);
		} while(value < chkValue);
		
		bRet = true;
    
		return bRet;
	}
	
	public boolean killOneDs()
	{
		boolean bRet = false;
		log.info("Kill one ds start ===>");
		AppServer cs = tfsGrid.getCluster(DSINDEX).getServer(0);
		bRet = cs.stop(KillTypeEnum.NORMALKILL, WAITTIME);
		log.info("Kill one ds end ===>");
		return bRet;
	}
	
	public boolean startOneDs()
	{
		boolean bRet = false;
		log.info("Start one ds start ===>");
		AppServer cs = tfsGrid.getCluster(DSINDEX).getServer(0);
		bRet = cs.start();
		log.info("Start one ds end ===>");
		return bRet;
	}
	
	public boolean killSecondDs()
	{
		boolean bRet = false;
		log.info("Kill second ds start ===>");
		AppServer cs = tfsGrid.getCluster(DSINDEX).getServer(1);
		bRet = cs.stop(KillTypeEnum.NORMALKILL, WAITTIME);
		log.info("Kill second ds end ===>");
		return bRet;
	}
	
	public boolean killAllDsOneSide()
	{
		boolean bRet = false;
		log.info("Kill all ds on one side start ===>");
		AppCluster csCluster = tfsGrid.getCluster(DSINDEX);
		for(int iLoop = 0; iLoop < csCluster.getServerList().size(); iLoop ++)
		{
			AppServer cs = csCluster.getServer(iLoop);
			bRet = cs.stop(KillTypeEnum.NORMALKILL, WAITTIME);
			if (bRet == false)
			{
				break;
			}
		}
		log.info("Kill all ds on one side end ===>");
		return bRet;
	}
	
	public boolean startAllDsOneSide()
	{
		boolean bRet = false;
		log.info("Start all ds on one side start ===>");
		AppCluster csCluster = tfsGrid.getCluster(DSINDEX);
		for(int iLoop = 0; iLoop < csCluster.getServerList().size(); iLoop ++)
		{
			AppServer cs = csCluster.getServer(iLoop);
			bRet = cs.stop(KillTypeEnum.NORMALKILL, WAITTIME);
			if (bRet == false)
			{
				break;
			}
		}
		log.info("Start all ds on one side end ===>");
		return bRet;
	}
	
	public boolean killAllDs()
	{
		boolean bRet = false;
		log.info("Kill all ds start ===>");
		for (int iLoop = DSINDEX; iLoop <= DSINDEXI; iLoop ++)
		{
			AppCluster csCluster = tfsGrid.getCluster(iLoop);
			for(int jLoop = 0; jLoop < csCluster.getServerList().size(); jLoop ++)
			{
				AppServer cs = csCluster.getServer(jLoop);
				bRet = cs.stop(KillTypeEnum.NORMALKILL, WAITTIME);
				if (bRet == false)
				{
					break;
				}
			}
		}
		log.info("Kill all ds end ===>");
		return bRet;
	}
	
	public boolean startAllDs()
	{
		boolean bRet = false;
		log.info("Start all ds start ===>");
		for (int iLoop = DSINDEX; iLoop <= DSINDEXI; iLoop ++)
		{
			AppCluster csCluster = tfsGrid.getCluster(iLoop);
			for(int jLoop = 0; jLoop < csCluster.getServerList().size(); jLoop ++)
			{
				AppServer cs = csCluster.getServer(jLoop);
				bRet = cs.start();
				if (bRet == false)
				{
					break;
				}
			}
		}
		log.info("Start all ds end ===>");
		return bRet;
	}
	
	public boolean cleanOneDs()
	{
		boolean bRet = false;
		log.info("Clean one ds start ===>");
		AppServer cs = tfsGrid.getCluster(DSINDEX).getServer(0);
		bRet = cs.stop(KillTypeEnum.NORMALKILL, WAITTIME);
		if (bRet == false) return bRet;
		
		bRet = cs.clean();
		log.info("Clean one ds end ===>");
		return bRet;
	}
	
	public boolean cleanAllDsOneSide()
	{
		boolean bRet = false;
		log.info("Clean all ds on one side start ===>");
		AppCluster csCluster = tfsGrid.getCluster(DSINDEX);
		for(int iLoop = 0; iLoop < csCluster.getServerList().size(); iLoop ++)
		{
			AppServer cs = csCluster.getServer(iLoop);
			bRet = cs.stop(KillTypeEnum.NORMALKILL, WAITTIME);
			if (bRet == false) break;

			bRet = cs.clean();
			if (bRet == false) break;
		}
		log.info("Clean all ds on one side end ===>");
		return bRet;
	}
	
	public boolean cleanAllDs()
	{
		boolean bRet = false;
		log.info("Clean all ds start ===>");
		for (int iLoop = DSINDEX; iLoop <= DSINDEXI; iLoop ++)
		{
			AppCluster csCluster = tfsGrid.getCluster(iLoop);
			for(int jLoop = 0; jLoop < csCluster.getServerList().size(); jLoop ++)
			{
				AppServer cs = csCluster.getServer(jLoop);
				bRet = cs.stop(KillTypeEnum.NORMALKILL, WAITTIME);
				if (bRet == false) break;
				
				bRet = cs.clean();
				if (bRet == false) break;
			}
		}
		log.info("Clean all ds end ===>");
		return bRet;
	}
}
