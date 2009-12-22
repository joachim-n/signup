<?php
// $Id: signup_form.inc,v 1.5 2009/01/22 18:59:29 dww Exp $


/**
 * @file
 * Site-specific code related to the form when users signup for a node.
 */

/**
 * Returns the value to use for the user name for anonymous signups.
 *
 * WARNING: If you are using a custom signup form other than the basic one
 * supplied with this module, you will need to alter this function to work
 * correctly.
 *
 * This value is used for the %user_name email token for anonymous users, and
 * also to identify a particular anonymous signup in various places in the UI.
 *
 * @param $form_data
 *   Array of custom signup form values for the current signup.
 * @param $email
 *   E-mail address of the anonymous user who signed up.
 * @return
 *   A string with the proper value for the %user_name email token.
 *
 * @see theme_signup_user_form()
 */
function theme_signup_anonymous_username($form_data, $email) {
  // In some cases, the best you can do is to use the anonymous user's
  // supplied email address, in which case, you should uncomment this:
  //return $email;

  // WARNING: This line is only valid if you are using the basic custom form
  // or if your own custom form has a 'Name' field.
  return $form_data['Name'];
}

