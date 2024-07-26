
static int mxc_isi_s_ext_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mxc_isi_cap_dev *isi_cap = ctrl_to_isi_cap(ctrl);
	struct mxc_isi_dev *mxc_isi = mxc_isi_get_hostdata(isi_cap->pdev);
	unsigned long flags;

	//spin_lock_irqsave(&mxc_isi->slock, flags);

	switch (ctrl->id) {
	case V4L2_CID_FRAME_IDX_CLR:
		isi_cap->frame_idx_clr = true;
		break;
		
	default:
		break;
	}

	//spin_unlock_irqrestore(&mxc_isi->slock, flags);
	return 0;
}

static int mxc_isi_g_ext_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mxc_isi_cap_dev *isi_cap = ctrl_to_isi_cap(ctrl);
	struct mxc_isi_dev *mxc_isi = mxc_isi_get_hostdata(isi_cap->pdev);
	unsigned long flags;

	spin_lock_irqsave(&mxc_isi->slock, flags);

	switch (ctrl->id) {
	case V4L2_CID_FRAME_IDX_CLR:
		ctrl->val = isi_cap->frame_idx;
		break;
		
	default:
		break;
	}

	spin_unlock_irqrestore(&mxc_isi->slock, flags);
	return 0;
}

static const struct v4l2_ctrl_ops mxc_isi_ext_ctrl_ops = {
	.s_ctrl = mxc_isi_s_ext_ctrl,
	.g_volatile_ctrl = mxc_isi_g_ext_ctrl,
};

static const struct v4l2_ctrl_config mxc_isi_ctrl_idx_clr_cfg = {
        .ops   = &mxc_isi_ext_ctrl_ops,
        .id    = V4L2_CID_FRAME_IDX_CLR,
        .name  = "Clear Frame Idx",
        .type  = V4L2_CTRL_TYPE_BOOLEAN,
        .flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
        .min   = 0,
        .max   = 1,
        .step  = 1,
        .def   = 0,
};

static const struct v4l2_ctrl_config mxc_isi_ctrl_idx_get_cfg = {
        .ops   = &mxc_isi_ext_ctrl_ops,
        .id    = V4L2_CID_FRAME_IDX_GET,
        .name  = "Clear Frame Idx",
        .type  = V4L2_CTRL_TYPE_INTEGER,
        .flags = V4L2_CTRL_FLAG_VOLATILE,
        .min   = 0,
        .max   = 65536,
        .step  = 1,
};


/*STEP1: Define V4L2_CID_  that must after USER BASE */
/*#define V4L2_CID_MY_CUSTOM_BASE (V4L2_CID_USER_BASE + 0x1200)*/
/*#define V4L2_CID_FRAME_IDX_CLR V4L2_CID_MY_CUSTOM_BASE+0 */
/*#define V4L2_CID_FRAME_IDX_GET V4L2_CID_MY_CUSTOM_BASE+1 */


/*STEP2: static const struct v4l2_ctrl_config, need volatile to update imediately*/

/*STEP3: Create the handler, and check handler->error*/
/*The number of control needs to be requested number of control plus 1*/
v4l2_ctrl_handler_init(handler, 2+1);
ctrls->rstid = v4l2_ctrl_new_custom(handler, &mxc_isi_ctrl_idx_clr_cfg, NULL);
ctrls->getid = v4l2_ctrl_new_custom(handler, &mxc_isi_ctrl_idx_get_cfg, NULL);
return handler->error;

/*STEP4: (Optional) Assign to video_device so that we can open /dev/videoX to get control */
struct video_device *vdev;
vdev->ctrl_handler = &isi_cap->ctrls.handler;

/*STEP5: Free control on error or exit*/
v4l2_ctrl_handler_free(&ctrls->handler);

