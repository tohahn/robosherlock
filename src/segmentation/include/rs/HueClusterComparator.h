/*
 * Software License Agreement (BSD License)
 *
 *  Point Cloud Library (PCL) - www.pointclouds.org
 *  Copyright (c) 2010-2012, Willow Garage, Inc.
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the copyright holder(s) nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE. 
 *
 *
 *
 */

#include <pcl/segmentation/boost.h>
#include <pcl/segmentation/comparator.h>
#include <stdlib.h>
#include <algorithm> 

namespace pcl
{
 /** \brief HueClusterComparator is a comparator to extract clusters based on euclidean distance + hue threshold
   * This needs to be run as a second pass after extracting planar surfaces, using MultiPlaneSegmentation for example.
   *
   * \author Tobias Hahn
   */
 template<typename PointT, typename PointNT, typename PointLT>
 class HueClusterComparator: public Comparator<PointT>
 {
 private:

 public:
     typedef typename Comparator<PointT>::PointCloud PointCloud;
     typedef typename Comparator<PointT>::PointCloudConstPtr PointCloudConstPtr;
     
     typedef typename pcl::PointCloud<PointNT> PointCloudN;
     typedef typename PointCloudN::Ptr PointCloudNPtr;
     typedef typename PointCloudN::ConstPtr PointCloudNConstPtr;
     
     typedef typename pcl::PointCloud<PointLT> PointCloudL;
     typedef typename PointCloudL::Ptr PointCloudLPtr;
     typedef typename PointCloudL::ConstPtr PointCloudLConstPtr;

     typedef boost::shared_ptr<HueClusterComparator<PointT, PointNT, PointLT> > Ptr;
     typedef boost::shared_ptr<const HueClusterComparator<PointT, PointNT, PointLT> > ConstPtr;

     using pcl::Comparator<PointT>::input_;
     
     /** \brief Empty constructor for HueClusterComparator. */
     HueClusterComparator ()
       : normals_ ()
       , angular_threshold_ (0.0f)
       , distance_threshold_ (0.005f)
       , depth_dependent_ ()
       , z_axis_ ()
       , hue_threshold_ (10.0f)
	   , interval_ (9)
	   , discretize_ (false)
     {
     }
     
     /** \brief Destructor for HueClusterComparator. */
     virtual
     ~HueClusterComparator ()
     {
     }

     virtual void 
     setInputCloud (const PointCloudConstPtr& cloud)
     {
       input_ = cloud;
       Eigen::Matrix3f rot = input_->sensor_orientation_.toRotationMatrix ();
       z_axis_ = rot.col (2);
     }
     
     /** \brief Provide a pointer to the input normals.
       * \param[in] normals the input normal cloud
       */
       inline void
       setInputNormals (const PointCloudNConstPtr &normals)
       {
         normals_ = normals;
       }
 
       /** \brief Get the input normals. */
       inline PointCloudNConstPtr
       getInputNormals () const
       {
         return (normals_);
       }

	   /** \brief Set the flag for discretization of hue values
	     * \param[in] The value to set it to
		 */
		 inline void setDiscretization(bool set) {
		 	discretize_ = set;
		 }
 
       /** \brief Set the tolerance in radians for difference in normal direction between neighboring points, to be considered part of the same plane.
         * \param[in] angular_threshold the tolerance in radians
         */
       virtual inline void
       setAngularThreshold (float angular_threshold)
       {
         angular_threshold_ = cosf (angular_threshold);
       }
       
       /** \brief Get the angular threshold in radians for difference in normal direction between neighboring points, to be considered part of the same plane. */
       inline float
       getAngularThreshold () const
       {
         return (acos (angular_threshold_) );
       }
 
       /** \brief Set the tolerance in meters for difference in perpendicular distance (d component of plane equation) to the plane between neighboring points, to be considered part of the same plane.
         * \param[in] distance_threshold the tolerance in meters 
         * \param depth_dependent
         */
       inline void
       setDistanceThreshold (float distance_threshold, bool depth_dependent)
       {
         distance_threshold_ = distance_threshold;
         depth_dependent_ = depth_dependent;
       }
 
       /** \brief Get the distance threshold in meters (d component of plane equation) between neighboring points, to be considered part of the same plane. */
       inline float
       getDistanceThreshold () const
       {
         return (distance_threshold_);
       }

       /** \brief Set the tolerance in points for difference in hue between neighboring points, to be considered part of the same plane.
         * \param[in] hue_threshold the tolerance in points
         */
       inline void
       setHueThreshold (float hue_threshold)
       {
         hue_threshold_ = hue_threshold;
		 interval_ = (int) hue_threshold - 1;
		 if (interval_ <= 0) {
		 	outInfo("Interval for discretization to low, set to 1!");
			interval_ = 1;
		 }
       }
 
       /** \brief Get the hue threshold in points between neighboring points, to be considered part of the same plane. */
       inline float
       getHueThreshold () const
       {
         return (hue_threshold_);
       }
 
       /** \brief Compare points at two indices by their plane equations.  True if the angle between the normals is less than the angular threshold,
         * and the difference between the d component of the normals is less than distance threshold, else false
         * \param idx1 The first index for the comparison
         * \param idx2 The second index for the comparison
         */
       virtual bool
       compare (int idx1, int idx2) const
       {
				 int hue1 = input_->points[idx1].h;
	 			 int hue2 = input_->points[idx2].h;
				 if (discretize_) {
				 	hue1 = hue1 - (hue1 % interval_);
					hue2 = hue2 - (hue2 % interval_);
				 }
	 			 int diff = std::abs(hue1 - hue2);
					
	 			 if (hue_threshold_ < std::min(diff, 360-diff))
	   		   return false;

         float dist_threshold = distance_threshold_;
         if (depth_dependent_)
         {
           Eigen::Vector3f vec = input_->points[idx1].getVector3fMap ();
           float z = vec.dot (z_axis_);
           dist_threshold *= z * z;
         }

         float dx = input_->points[idx1].x - input_->points[idx2].x;
         float dy = input_->points[idx1].y - input_->points[idx2].y;
         float dz = input_->points[idx1].z - input_->points[idx2].z;
         float dist = sqrtf (dx*dx + dy*dy + dz*dz);
					
         return (dist < dist_threshold);
       }
       
     protected:
       PointCloudNConstPtr normals_;
 
       float angular_threshold_;
       float distance_threshold_;
       bool depth_dependent_;
       Eigen::Vector3f z_axis_;
       float hue_threshold_;
	   int interval_;
	   bool discretize_;
   };
 }
