#!/bin/sh
# Runs makeflow directions on ec2 instance
#
# Invocation:
# $ ./amazon_ec2.sh $AWS_ACCESS_KEY $AWS_SECRET_KEY
set -e
EC2_TOOLS_DIR="../ec2-api-tools-1.7.5.1/bin"
AMI_IMAGE="ami-4b630d2e"
INSTANCE_TYPE="t1.micro"
USERNAME="ubuntu"
AWS_ACCESS_KEY=$1
AWS_SECRET_KEY=$2
KEYPAIR_NAME="makeflow-keypair"
SECURITY_GROUP_NAME="makeflow-security-group"

cleanup () {
    echo "Deleting temporary security group..."
    $EC2_TOOLS_DIR/ec2-delete-group $SECURITY_GROUP_NAME > /dev/null
    echo "Temporary security group deleted."

    echo "Deleting temporary keypair..."
    $EC2_TOOLS_DIR/ec2-delete-keypair $KEYPAIR_NAME > /dev/null
    rm -f $KEYPAIR_NAME.pem
    echo "Temporary keypair deleted."
}

run_ssh_cmd () {
    ssh -o StrictHostKeyChecking=no -i $KEYPAIR_NAME.pem $USERNAME@$PUBLIC_DNS \
            "$1"
}

get_file_from_server_to_destination () {
    echo "Copying file to $(pwd)"
    scp -o StrictHostKeyChecking=no -i $KEYPAIR_NAME.pem \
        $USERNAME@$PUBLIC_DNS:~/"$1" $2
}

generate_temp_keypair () {
    # Generate temp key pair and save
    echo "Generating temporary keypair..."
    $EC2_TOOLS_DIR/ec2-create-keypair $KEYPAIR_NAME | sed 's/.*KEYPAIR.*//' > $KEYPAIR_NAME.pem
    echo "Keypair generated."
}

create_temp_security_group () {
    # Create temp security group
    echo "Generating temporary security group..."
    $EC2_TOOLS_DIR/ec2-create-group $SECURITY_GROUP_NAME -d "$SECURITY_GROUP_NAME"
    echo "Security group generated."
}

authorize_port_22_for_ssh_access () {
    echo "Authorizing port 22 on instance for SSH access..."
    $EC2_TOOLS_DIR/ec2-authorize $SECURITY_GROUP_NAME -p 22
}

trap cleanup EXIT

if [ "$#" -ne 2 ]; then
    echo "Incorrect arguments passed to program"
    echo "Usage: $0 AWS_ACCESS_KEY AWS_SECRET_KEY" >&2
    exit 1
fi

generate_temp_keypair
create_temp_security_group
authorize_port_22_for_ssh_access

echo "Starting EC2 instance..."
INSTANCE_ID=$($EC2_TOOLS_DIR/ec2-run-instances \
    $AMI_IMAGE \
    -t $INSTANCE_TYPE \
    -k $KEYPAIR_NAME \
    -g $SECURITY_GROUP_NAME \
    | grep "INSTANCE" | awk '{print $2}')

INSTANCE_STATUS="pending"
while [ "$INSTANCE_STATUS" = "pending" ]; do
    INSTANCE_STATUS=$($EC2_TOOLS_DIR/ec2-describe-instances $INSTANCE_ID \
    | grep "INSTANCE" | awk '{print $5}')
done

PUBLIC_DNS=$($EC2_TOOLS_DIR/ec2-describe-instances $INSTANCE_ID \
| grep "INSTANCE" | awk '{print $4'})

chmod 400 $KEYPAIR_NAME.pem

# Try for successful ssh connection
tries="10"
SUCCESSFUL_SSH=-1
while [ $tries -ne 0 ]
do
    run_ssh_cmd "echo 'Connection to remote server successful'" \
        && SUCCESSFUL_SSH=0 && break
    tries=$[$tries-1]
    sleep 1
done

# Run rest of ssh commands
if [ $SUCCESSFUL_SSH -eq 0 ]
then
    run_ssh_cmd "echo 'dat thang' > testfile"

    # Get output files
    get_file_from_server_to_destination "testfile" /tmp
fi

echo "Terminating EC2 instance..."
$EC2_TOOLS_DIR/ec2-terminate-instances $INSTANCE_ID
