#!/bin/sh
#
# dtnmenu - scripts launched at runnig of dtn_tune
# to do routine admintstative tasks
#

# when viewing this script, set tabstops to 4

# The main script is at the bottom, after all support functions are defined

return='Return to previous menu'
select_choice='Enter option : '

clear_screen()
{
	tput clear
}

enter_to_continue()
{
	printf '\n\t%s' "Hit ENTER to continue: "
	read junk
}

prune_logs()
{
	clear_screen
	printf '\n\t%s\n\t%s\n' \
		"This will remove all the old tuningLog log files except the" \
		"current one."
	printf '\n\t%s (y/n): ' \
		"Do you wish to continue? "
	read answer
	if [ "$answer" != 'y' -a "$answer" != 'Y' ]
	then
		enter_to_continue
		return 0
	fi
	rm -rf /tmp/tuningLogOld.*
	echo 0 > /tmp/tuningLog.count
	enter_to_continue
	return 0
}

apply_recommended_settings()
{
	clear_screen
	if [ -f  /tmp/applyDefFile ]
	then
		printf '\n\t%s\n' \
			"You are attempting to apply the Last Tuning Recommendations" 
		printf '\n\t%s (y/n): ' \
			"Do you wish to continue? "
		read answer
		if [ "$answer" != 'y' -a "$answer" != 'Y' ]
		then
			enter_to_continue
			return 0
		fi

		sysctlmodified=0
		
		printf '\n\t%s (y/n): ' \
			"Do you wish to apply the Tuning Recommendations permanently? " 
		read answer
		if [ "$answer" != 'y' -a "$answer" != 'Y' ]
		then
			printf '\n###%s\n\n' "Applying Tuning Recommendations until a reboot..."
		else
			printf '\n###%s\n\n' "Applying Tuning Recommendations Permanently..."
			sysctlmodified=1
		fi

		nlines=`sed -n '1p' /tmp/applyDefFile`	
		count=2

		if [ ${sysctlmodified} -eq 1 ]
		then
			echo "#Start of tuningMod modifications" >> /etc/sysctl.conf	
		fi

		while [ ${count} -lt ${nlines} ]
		do
			linenum=`sed -n "${count}p" /tmp/applyDefFile`
			echo $linenum > /tmp/tun_app_command
			sh /tmp/tun_app_command
		
			if [ ${sysctlmodified} -eq 1 ]
			then
			echo "$linenum >> /etc/sysctl.conf" > /tmp/tun_app_command
			sh /tmp/tun_app_command
			fi

			count=`expr $count + 1`
		done	
		
		if [ ${sysctlmodified} -eq 1 ]
		then
			echo "#End of tuningMod modifications" >> /etc/sysctl.conf	
		fi

		rm -f /tmp/tun_app_command
		rm -f /tmp/applyDefFile
	else
		printf '\n###%s\n\n' "Sorry. You do not have any Tuning Recommendations to apply..."

	fi
    enter_to_continue
	return 0
}

run_dtntune()
{
logcount=
	clear_screen
	printf '\n###%s\n\n' "Running Tuning Assessment..."
	if [ -f /tmp/tuningLog ]
	then
		logcount=`cat /tmp/tuningLog.count`
		cp /tmp/tuningLog /tmp/tuningLogOld.$logcount
		logcount=`expr $logcount + 1`
		echo $logcount > /tmp/tuningLog.count
	else
		echo 0 > /tmp/tuningLog.count
	fi
	./dtn_tune $1

	if [ $? = 0 ]
	then	
		more -d /tmp/tuningLog	
		printf '\n###%s' "This output has been saved in /tmp/tuningLog"
	else
		more -d /tmp/tuningLog	
		printf '\n###%s' "This output has been saved in /tmp/tuningLog"
		echo >&2
    	echo >&2 $UsageString
	fi
	enter_to_continue
	return 0
}

UsageString="\
Usage:\\t[sudo ./dtnmenu]\\t\\t- Configure Tunables \\n\
\\t[sudo ./dtnmenu <device>]\\t- Configure Tunables and Device"

# main execution thread

	
	repeat_main=1
	while  [ $repeat_main = 1 ]
	do
		clear_screen
		printf '\n\n\t%s\n\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s' \
			"DTN Tuning Utility" \
			"1) Run DTN Tune" \
			"2) Apply Last Recommended Tuning Values" \
			"3) Prune Old Logs" \
			"4) Escape to Linux Shell" \
			"5) Exit" \
			"$select_choice"

		read answer
		case "$answer" in
			1)
				run_dtntune "$1"
				;;
			2)
				apply_recommended_settings
				;;
			3)
				prune_logs
				;;
			4)
				clear_screen
				$SHELL
				clear_screen
				enter_to_continue
				;;
			q|5)
				clear_screen
				exit 0
				;;
			*)
				;;
		esac
	done
	echo
