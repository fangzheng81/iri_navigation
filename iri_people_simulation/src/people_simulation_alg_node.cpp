#include "people_simulation_alg_node.h"

PeopleSimulationAlgNode::PeopleSimulationAlgNode(void) :
  algorithm_base::IriBaseAlgorithm<PeopleSimulationAlgorithm>(),
  n_persons_(0), simulation_mode_( PeopleSimulationAlgNode::Normal ),
  tf_listener_(ros::Duration(10.f))
{
  //init class attributes if necessary
  this->loop_rate_ = 10;//in [Hz]

  // [init publishers]
  this->tracksMarkers_publisher_ = this->public_node_handle_.advertise<visualization_msgs::MarkerArray>("tracksMarkers", 100);
  this->tracks_publisher_ = this->public_node_handle_.advertise<iri_perception_msgs::peopleTrackingArray>("tracks", 100);
  
  
  // [init services]
  this->reset_server_ = this->public_node_handle_.advertiseService("reset", &PeopleSimulationAlgNode::resetCallback, this);
  
  // [init clients]
  
  // [init action servers]
  
  // [init action clients]
  
  init_sim();
}

PeopleSimulationAlgNode::~PeopleSimulationAlgNode(void)
{
  // [free dynamic memory]
}

void PeopleSimulationAlgNode::init_sim()
{
  scene_.set_dt( 0.1 );
	//this->public_node_handle_.getParam("simulation_mode", simulation_mode_);
  this->public_node_handle_.getParam("number_persons", n_persons_);
  this->public_node_handle_.getParam("force_map_path", force_map_path_);
  this->public_node_handle_.getParam("destination_map_path", destination_map_path_);
  this->public_node_handle_.getParam("remove_targets", remove_targets_);
  this->public_node_handle_.getParam("robot", robot_);

	scene_.set_number_virtual_people( n_persons_ );
  scene_.set_remove_targets( remove_targets_ );
	peopleTrackingArray_msg_.header.frame_id = "/map";

  //read destinations
	if ( !scene_.read_destination_map(  destination_map_path_.c_str() ) )
	{
		ROS_ERROR("Could not read map destinations file !!!");
	}
  else{
		ROS_WARN("read destinations map file : SUCCESS!!!");
	}

  //read force map
	if ( !scene_.read_force_map(  force_map_path_.c_str() ) )
	{
		ROS_ERROR("Could not read map force file !!!");
	}
  else{
		ROS_WARN("read map force file : SUCCESS!!!");
	}
	//false = no destroy simulated persons
	//scene_.set_remove_targets(true);

	//target marker
	track_marker_.header.frame_id = "/map";
	track_marker_.ns = "tracks";
	track_marker_.type = visualization_msgs::Marker::CYLINDER;
	track_marker_.action = visualization_msgs::Marker::ADD;
	track_marker_.lifetime = ros::Duration(1.0f);
	track_marker_.scale.x = 0.5;
	track_marker_.scale.y = 0.5;
	track_marker_.scale.z = 0.6;
	track_marker_.color.a = 0.6;
	track_marker_.color.r = 0.0;
	track_marker_.color.g = 1.0;
	track_marker_.color.b = 0.0;
	track_marker_.pose.position.z = 0.3;

	id_marker_.header.frame_id = "/map";
	id_marker_.ns = "ids";
	id_marker_.type = visualization_msgs::Marker::TEXT_VIEW_FACING;
	id_marker_.action = visualization_msgs::Marker::ADD;
	id_marker_.lifetime = ros::Duration(1.0f);
	id_marker_.scale.z = 0.5;
	id_marker_.pose.position.z = 1.0;
	id_marker_.color.a = 0.5;
	id_marker_.color.r = 0.0;
	id_marker_.color.g = 0.0;
	id_marker_.color.b = 0.0;

}

void PeopleSimulationAlgNode::mainNodeThread(void)
{
  // [fill msg structures]
  //peopleTrackingArray_msg_.header.seq = seq_cont_; //autocompeted by ros.... lol
  peopleTrackingArray_msg_.header.stamp = ros::Time::now();
  peopleTrackingArray_msg_.peopleSet.clear();
  //for every new trajectory
	iri_perception_msgs::peopleTracking person;


	//scene update
    this->alg_.lock();
// ------------------------------------------------------------------------
  //Get Robot position: transform empty/zero pose from base_link to map
  std::string target_frame = "/map";
  //TODO set param in .launch  <param name="~/robot" type="string" value="$(env ROBOT)" />
  std::string source_frame = "/" + this->robot_ + "/base_link"; 
  ros::Time target_time    = ros::Time::now();
  bool tf_exists = tf_listener_.waitForTransform(target_frame, source_frame, target_time, ros::Duration(1), ros::Duration(0.01));
  if(tf_exists)
  {
    geometry_msgs::PoseStamped poseIn;
    geometry_msgs::PoseStamped poseOut;
    poseIn.header.stamp    = target_time;
    poseIn.header.frame_id = source_frame;
    poseIn.pose.orientation.z = 1.0; //valid quaternion
    tf_listener_.transformPose(target_frame, poseIn, poseOut);
    //TODO add velocities in /map coordinates to the scene update
    scene_.update_robot(Spose(poseOut.pose.position.x, poseOut.pose.position.y, poseOut.header.stamp.toSec()));
  }
  else
  {
    ROS_INFO("fail, no transform found");
  }


	std::vector<SdetectionObservation> obs_scene;
	obs_scene.push_back( SdetectionObservation(0, ros::Time::now().toSec() ));//void observation, just for the timestamp
	scene_.update_scene( obs_scene );

  //publish data
  //scene_.get_scene_observation(scene_targets_pointV,scene_targets_ids);
  const std::list<Cperson_abstract *>* person_list = scene_.get_scene( );
  for( std::list<Cperson_abstract*>::const_iterator iit = person_list->begin();
        iit!=person_list->end(); iit++ )
  {
		person.targetId = (*iit)->get_id();
		person.x = (*iit)->get_current_pointV().x;
		person.y = (*iit)->get_current_pointV().y;
		person.vy = (*iit)->get_current_pointV().vx;
		person.vx = (*iit)->get_current_pointV().vy;
  	person.covariances[0] = (*iit)->get_current_pointV().cov[0];
  	person.covariances[1] = (*iit)->get_current_pointV().cov[1];
  	person.covariances[4] = (*iit)->get_current_pointV().cov[4];
  	person.covariances[5] = (*iit)->get_current_pointV().cov[5];
		peopleTrackingArray_msg_.peopleSet.push_back(person);
	}

	if( vis_mode_ )
	{
		MarkerArray_msg_.markers.clear();
		track_marker_.header.stamp =   peopleTrackingArray_msg_.header.stamp;
		id_marker_.header.stamp =   peopleTrackingArray_msg_.header.stamp;
    // draw targets
    for( std::list<Cperson_abstract*>::const_iterator iit = person_list->begin();
          iit!=person_list->end(); iit++ )
    {
			track_marker_.id = (*iit)->get_id();
			track_marker_.pose.position.x = (*iit)->get_current_pointV().x;
			track_marker_.pose.position.y = (*iit)->get_current_pointV().y;
			MarkerArray_msg_.markers.push_back(track_marker_);
			id_marker_.id = (*iit)->get_id();
			id_marker_.pose.position.x = (*iit)->get_current_pointV().x;
			id_marker_.pose.position.y = (*iit)->get_current_pointV().y;
			std::ostringstream target_id;
			target_id << id_marker_.id;
			id_marker_.text = target_id.str();
			MarkerArray_msg_.markers.push_back(id_marker_);
		}
    //draw robot
		track_marker_.id = 0;
  	track_marker_.ns = "robot";
		track_marker_.pose.position.x = scene_.get_robot()->get_current_pointV().x;
		track_marker_.pose.position.y = scene_.get_robot()->get_current_pointV().y;
		MarkerArray_msg_.markers.push_back(track_marker_);
  	track_marker_.ns = "tracks";


		this->tracksMarkers_publisher_.publish(this->MarkerArray_msg_);

	}
  this->alg_.unlock();


  // [fill srv structure and make request to the server]
  
  // [fill action structure and make request to the action server]

  // [publish messages]
  this->tracks_publisher_.publish(this->peopleTrackingArray_msg_);
}

/*  [subscriber callbacks] */


/*  [service callbacks] */
bool PeopleSimulationAlgNode::resetCallback(std_srvs::Empty::Request &req, std_srvs::Empty::Response &res) 
{ 
  ROS_INFO("PeopleSimulationAlgNode::resetCallback: New Request Received!"); 

  //use appropiate mutex to shared variables if necessary 
  //this->reset_mutex_.enter(); 

  if( simulation_mode_ == PeopleSimulationAlgNode::Density_incremental )
  {
    //TODO: set the incremental proportion
    //n_persons_ = (num_experiment_/25 + 1)*25;
    //scene_.set_number_virtual_people( n_persons_ );
  }

  
  this->alg_.lock(); 
  ROS_INFO("PeopleSimulationAlgNode::resetCallback: Processin New Request!"); 
  //do operations with req and output on res 
  scene_.clear_scene();
  scene_.set_number_virtual_people( n_persons_ );
  this->alg_.unlock(); 

  //unlock previously blocked shared variables 
  //this->reset_mutex_.exit(); 

  return true; 
}

/*  [action callbacks] */

/*  [action requests] */

void PeopleSimulationAlgNode::node_config_update(Config &config, uint32_t level)
{
  this->alg_.lock();

	ROS_INFO("         *******  algorithm config update  *******\n\n");
    simulation_mode_ = (PeopleSimulationAlgNode::simulation_mode) config.simulation_mode;
    vis_mode_ = config.vis_mode;
	n_persons_ = config.number_persons;
	scene_.set_number_virtual_people( n_persons_ );
 
	
  this->alg_.unlock();
}

void PeopleSimulationAlgNode::addNodeDiagnostics(void)
{
}

/* main function */
int main(int argc,char *argv[])
{
  return algorithm_base::main<PeopleSimulationAlgNode>(argc, argv, "people_simulation_alg_node");
}
