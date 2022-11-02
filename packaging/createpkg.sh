echo " "
echo "Creating Tuning Module Assessment package..."
cp ../assess-tuning/dtn_tune .
cp ../assess-tuning/dtnmenu .
cp ../assess-tuning/dtn_menu.sh .
cp ../assess-tuning/gdv.sh .
cp ../assess-tuning/user_config.txt .
cp ../assess-tuning/common_irq_affinity.sh .
cp ../assess-tuning/set_irq_affinity.sh .
zip ASSESSdtntune.zip dtn_tune gdv.sh user_config.txt dtn_menu.sh dtnmenu readme.txt common_irq_affinity.sh set_irq_affinity.sh

rm dtn_tune 
rm dtnmenu 
rm dtn_menu.sh 
rm gdv.sh 
rm user_config.txt 
rm common_irq_affinity.sh 
rm set_irq_affinity.sh 

echo " "
echo "Creating Tuning Module StandAlone package..."
cp ../cli/tmp/tuncli .
cp ../userspace/user_dtn .
cp ../userspace/common_irq_affinity.sh .
cp ../userspace/set_irq_affinity.sh .
cp ../userspace/help_dtn.sh .
cp ../userspace/user_config.txt .
cp ../userspace/user_menu.sh .
cp ../userspace/gdv_100.sh .
cp ../userspace/gdv.sh .
cp ../userspace/readme.txt .
cp ../util/plotgraph.py .
cp ../util/conv_csv_to_json.py .
zip SAdtntune.zip help_dtn.sh user_config.txt user_menu.sh gdv_100.sh gdv.sh readme.txt common_irq_affinity.sh set_irq_affinity.sh plotgraph.py conv_csv_to_json.py


rm tuncli 
rm user_dtn 
rm common_irq_affinity.sh 
rm set_irq_affinity.sh 
rm help_dtn.sh 
rm user_config.txt 
rm user_menu.sh 
rm gdv_100.sh 
rm gdv.sh 
rm readme.txt 
rm plotgraph.py 
rm conv_csv_to_json.py 

